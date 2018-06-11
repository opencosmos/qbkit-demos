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
#include "Reactor.hpp"

#include "Bridge.hpp"

namespace Bridge {

static constexpr std::size_t CHUNK_SIZE = 0x10000;

class MessagePart
{
	static constexpr std::uint8_t FLAG_MORE = 0x01;
	static constexpr std::uint8_t FLAG_REPLY = 0x02;
	template <typename InputIt>
	MessagePart(InputIt begin, InputIt end, bool has_more, bool is_reply) :
		data(begin, end),
		has_more(has_more),
		is_reply(is_reply)
	{
	}
	template <typename InputIt>
	MessagePart(InputIt begin, InputIt end, std::uint8_t flags) :
		MessagePart(begin, end, flags & FLAG_MORE, flags & FLAG_REPLY)
	{
	}
public:
	MessagePart(const std::vector<std::uint8_t>& data) :
		MessagePart(data.cbegin() + 1, data.cend(), data[0])
	{
	}
	MessagePart(const zmq::message_t& msg, bool has_more, bool is_reply) :
		MessagePart(
			static_cast<const std::uint8_t *>(msg.data()),
			static_cast<const std::uint8_t *>(msg.data()) + msg.size(),
			has_more,
			is_reply)
	{
	}
	std::uint8_t flags() const
	{
		return (has_more ? FLAG_MORE : 0) | (is_reply ? FLAG_REPLY : 0);
	}
	operator std::vector<std::uint8_t> () const
	{
		std::vector<std::uint8_t> buf(data.size() + 1);
		buf[0] = flags();
		std::copy(data.cbegin(), data.cend(), buf.begin() + 1);
		return buf;
	}
	operator zmq::message_t () const
	{
		zmq::message_t part(data.size());
		std::copy(data.cbegin(), data.cend(), static_cast<std::uint8_t *>(part.data()));
		return part;
	}
	std::vector<std::uint8_t> data;
	bool has_more;
	bool is_reply;
};

class Task :
	public Util::Reactor
{
	Config config;

	std::optional<Posix::Serial> serial;

	zmq::socket_t router;
	zmq::socket_t dealer;

	Kiss::Encoder encoder;
	Kiss::Decoder decoder;

	std::deque<std::uint8_t> uart_tx_queue;
	std::deque<MessagePart> router_tx_queue;
	std::deque<MessagePart> dealer_tx_queue;

	std::vector<std::uint8_t> chunk;

	void serial_read();
	void serial_write();

	void socket_read(zmq::socket_t& socket, const std::string& name, bool is_reply);
	void socket_write(zmq::socket_t& socket, const std::string& name, std::deque<MessagePart>& tx_queue);

	void router_socket_read() { socket_read(router, "router", false); }
	void router_socket_write() { socket_write(router, "router", router_tx_queue); }

	void dealer_socket_read() { socket_read(dealer, "dealer", true); }
	void dealer_socket_write() { socket_write(dealer, "dealer", dealer_tx_queue); }

	template <typename InputIt>
	void emit_packets(InputIt begin, InputIt end);

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
	Util::Reactor(context, "bridge", config.verbose),
	config(config),
	serial(make_serial(config.device, config.baud)),
	router(context, ZMQ_ROUTER),
	dealer(context, ZMQ_DEALER),
	encoder(),
	decoder(config.max_packet_size),
	chunk(CHUNK_SIZE)
{
	dealer.setsockopt(ZMQ_LINGER, 0);
	router.setsockopt(ZMQ_LINGER, 0);

	dealer.bind(config.server_url);
	router.bind(config.client_url);
}

void Task::set_events()
{
	if (serial) {
		bind_read(serial->fileno());
		if (!uart_tx_queue.empty()) {
			bind_write(serial->fileno());
		}
	}

	bind_read(router);
	if (!router_tx_queue.empty()) {
		bind_write(router);
	}

	bind_read(dealer);
	if (!dealer_tx_queue.empty()) {
		bind_write(dealer);
	}
}

void Task::handle_events()
{
	if (is_readable(router)) {
		router_socket_read();
	}
	if (is_readable(dealer)) {
		dealer_socket_read();
	}
	if (serial) {
		if (is_writeable(serial->fileno())) {
			serial_write();
		}
		if (is_readable(serial->fileno())) {
			serial_read();
		}
	} else {
		emit_packets(uart_tx_queue.cbegin(), uart_tx_queue.cend());
	}
	if (is_writeable(router)) {
		router_socket_write();
	}
	if (is_writeable(dealer)) {
		dealer_socket_write();
	}
}

template <typename InputIt>
void Task::emit_packets(InputIt begin, InputIt end)
{
	auto emit = Util::GeneratorIterator<const std::vector<std::uint8_t>&>(
		[this] (const auto& packet) {
			MessagePart part(packet);
			if (part.is_reply) {
				router_tx_queue.emplace_back(std::move(part));
			} else {
				dealer_tx_queue.emplace_back(std::move(part));
			}
		});
	for (auto& it = begin; it != end; ) {
		it = decoder.decode_packet(it, end, emit);
	}
}

void Task::serial_read()
{
	chunk.clear();
	serial->read(chunk);
	emit_packets(chunk.cbegin(), chunk.cend());
	logger("Read ", chunk.size(), " bytes from serial");
}

void Task::serial_write()
{
	chunk.clear();
	const auto begin = uart_tx_queue.cbegin();
	const auto end = begin + std::min(chunk.capacity(), uart_tx_queue.size());
	std::copy(begin, end, std::back_inserter(chunk));
	auto written = serial->write(chunk);
	uart_tx_queue.erase(begin, begin + written);
	logger("Wrote ", written, " bytes from serial");
}

void Task::socket_read(zmq::socket_t& socket, const std::string& name, bool is_reply)
{
	zmq::message_t msg;
	if (!socket.recv(&msg)) {
		throw SystemError("Failed to receive ØMQ message via " + name);
	}
	MessagePart msg_part(msg, socket.getsockopt<int>(ZMQ_RCVMORE), is_reply);
	std::uint8_t flags = msg_part.flags();
	encoder.open(std::back_inserter(uart_tx_queue));
	encoder.write(flags, std::back_inserter(uart_tx_queue));
	encoder.write_n(static_cast<const std::uint8_t *>(msg_part.data.data()), msg_part.data.size(), std::back_inserter(uart_tx_queue));
	encoder.close(std::back_inserter(uart_tx_queue));
	logger("Received ", msg.size(), " bytes via ØMQ ", name, msg_part.has_more ? " [more]" : "");
}

void Task::socket_write(zmq::socket_t& socket, const std::string& name, std::deque<MessagePart>& tx_queue)
{
	const auto& msg_part = tx_queue.front();
	zmq::message_t msg(msg_part);
	if (!socket.send(std::move(msg), msg_part.has_more ? ZMQ_SNDMORE : 0)) {
		throw SystemError("Failed to send ØMQ message via " + name);
	}
	logger("Sent ", msg_part.data.size(), " byte packet via ØMQ ", name, msg_part.has_more ? " [more]" : "");
	tx_queue.pop_front();
}

Bridge::Bridge(const Config& config, zmq::context_t& ctx)
{
	worker = std::thread([&config, &ctx] () {
		Task reactor(config, ctx);
		reactor.run_loop();
	});
}

Bridge::~Bridge()
{
	worker.join();
}

}
