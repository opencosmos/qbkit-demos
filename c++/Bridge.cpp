#include <string>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <vector>
#include <deque>
#include <optional>

#include <fcntl.h>

#include <zmq.hpp>

#include "Kiss.hpp"
#include "Serial.hpp"
#include "GeneratorIterator.hpp"
#include "SystemError.hpp"
#include "Task.hpp"

#include "Bridge.hpp"

namespace Bridge {

static constexpr std::size_t CHUNK_SIZE = 0x10000;

static constexpr std::uint8_t FLAG_MORE = 0x01;

class Task :
	public Util::Task
{
	Config config;
	std::optional<Posix::Serial> serial;
	zmq::socket_t pub;
	zmq::socket_t sub;
	Kiss::Encoder encoder;
	Kiss::Decoder decoder;
	std::deque<std::uint8_t> uart_rx_buf;
	std::deque<std::uint8_t> uart_tx_buf;
	std::vector<std::uint8_t> chunk;

	const int pfd_pub;
	const int pfd_sub;
	const int pfd_serial;

	void serial_read();
	void serial_write();
	void pub_socket_write();
	void sub_socket_read();

protected:
	virtual void set_events() override;
	virtual void handle_events() override;

public:
	Task(const Config& config, zmq::context_t& context);
};

static std::optional<Posix::Serial> make_serial(const std::string& device, int baud)
{
	if (device.empty()) {
		return std::nullopt;
	} else {
		return std::make_optional<Posix::Serial>(Posix::File(device, O_RDWR), baud);
	}
}

Task::Task(const Config& config, zmq::context_t& context) :
	Util::Task(context, "bridge", config.verbose),
	config(config),
	serial(make_serial(config.device, config.baud)),
	pub(context, ZMQ_PUB),
	sub(context, ZMQ_SUB),
	encoder(),
	decoder(config.max_packet_size),
	uart_rx_buf(),
	uart_tx_buf(),
	chunk(CHUNK_SIZE),
	pfd_pub(bind(pub)),
	pfd_sub(bind(sub)),
	pfd_serial(bind(serial ? serial->fileno() : -1))
{
	/* Configure sockets */
	pub.setsockopt(ZMQ_LINGER, 0);
	sub.setsockopt(ZMQ_LINGER, 0);

	pub.bind(config.rx_url);
	sub.bind(config.tx_url);

	sub.setsockopt(ZMQ_SUBSCRIBE, "", 0);
}

void Task::set_events()
{
	get_pfd(pfd_serial).events = serial ? ZMQ_POLLIN | (uart_tx_buf.empty() ? 0 : ZMQ_POLLOUT) : 0;
	get_pfd(pfd_pub).events = uart_rx_buf.empty() ? 0 : ZMQ_POLLOUT;
	get_pfd(pfd_sub).events = ZMQ_POLLIN;
}

void Task::handle_events()
{
	if (get_pfd(pfd_sub).revents & ZMQ_POLLIN) {
		sub_socket_read();
	}
	if (get_pfd(pfd_serial).revents & ZMQ_POLLOUT) {
		serial_write();
	}
	if (!serial) {
		std::copy(uart_tx_buf.cbegin(), uart_tx_buf.cend(), std::back_inserter(uart_rx_buf));
		uart_tx_buf.clear();
	}
	if (get_pfd(pfd_serial).revents & ZMQ_POLLIN) {
		serial_read();
	}
	if (get_pfd(pfd_pub).revents & ZMQ_POLLOUT) {
		pub_socket_write();
	}
}

void Task::serial_read()
{
	chunk.clear();
	serial->read(chunk);
	std::copy(chunk.cbegin(), chunk.cend(), std::back_inserter(uart_rx_buf));
	logger("Read ", chunk.size(), " bytes from serial");
}

void Task::serial_write()
{
	chunk.clear();
	const auto begin = uart_tx_buf.cbegin();
	const auto end = begin + std::min(chunk.capacity(), uart_tx_buf.size());
	std::copy(begin, end, std::back_inserter(chunk));
	uart_tx_buf.erase(begin, end);
	serial->write(chunk);
	logger("Wrote ", chunk.size(), " bytes from serial");
}

void Task::pub_socket_write()
{
	auto it = decoder.decode_packet(uart_rx_buf.cbegin(), uart_rx_buf.cend(), Util::GeneratorIterator<const std::vector<std::uint8_t>&>(
		[this] (const auto& packet) {
			zmq::message_t msg(packet.size() - 1);
			bool more = (packet[0] & FLAG_MORE) != 0;
			std::copy(packet.cbegin() + 1, packet.cend(), static_cast<std::uint8_t *>(msg.data()));
			if (!pub.send(msg, more ? ZMQ_SNDMORE : 0)) {
				throw SystemError("Failed to publish ØMQ message");
			}
			logger("Sent ", packet.size() - 1, " bytes via ØMQ", more ? " [more]" : "");
		}));
	uart_rx_buf.erase(uart_rx_buf.cbegin(), it);
}

void Task::sub_socket_read()
{
	zmq::message_t msg;
	if (!sub.recv(&msg)) {
		throw SystemError("Failed to receive ØMQ message");
	}
	/* Is there more to come? */
	const auto more = sub.getsockopt<int>(ZMQ_RCVMORE);
	std::uint8_t flags = 0x00 | (more ? FLAG_MORE : 0);
	/* Write status flags and message data */
	encoder.open(std::back_inserter(uart_tx_buf));
	encoder.write(flags, std::back_inserter(uart_tx_buf));
	encoder.write_n(static_cast<const std::uint8_t *>(msg.data()), msg.size(), std::back_inserter(uart_tx_buf));
	encoder.close(std::back_inserter(uart_tx_buf));
	logger("Received ", msg.size(), " bytes via ØMQ", more ? " [more]" : "");
}

Bridge::Bridge(const Config& config, zmq::context_t& ctx)
{
	worker = std::thread([&config, &ctx] () {
		Task task(config, ctx);
		task.run_loop();
	});
}

Bridge::~Bridge()
{
	worker.join();
}

}
