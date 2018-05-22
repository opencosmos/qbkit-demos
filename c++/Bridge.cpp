#include <string>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <vector>
#include <deque>

#include <signal.h>
#include <sys/poll.h>
#include <sys/signalfd.h>

#include <zmq.hpp>

#include "Kiss.hpp"
#include "Serial.hpp"
#include "Cork.hpp"
#include "GeneratorIterator.hpp"
#include "SignalReceiver.hpp"

#include "Bridge.hpp"

namespace Bridge {

static constexpr std::size_t CHUNK_SIZE = 0x10000;

class BridgeTask
{
	Config config;
	Posix::SignalReceiver sr;
	Posix::Serial serial;
	zmq::context_t context;
	zmq::socket_t pub;
	zmq::socket_t sub;
	Kiss::Encoder encoder;
	Kiss::Decoder decoder;
	std::deque<std::uint8_t> uart_rx_buf;
	std::deque<std::uint8_t> uart_tx_buf;
	std::vector<std::uint8_t> chunk;

	bool exit;

	enum pfds {
		pfd_signal = 0,
		pfd_serial = 1,
		pfd_pub_socket = 2,
		pfd_sub_socket = 3,
		pfd_count = 4
	};
	zmq_pollitem_t pfds[pfd_count];

	void signal_read();
	void serial_read();
	void serial_write();
	void pub_socket_write();
	void sub_socket_read();
	void run();

public:
	BridgeTask(const Config& config);
	void run_loop();
};

BridgeTask::BridgeTask(const Config& config) :
	config(config),
	sr(SIGUSR1, SIGINT, SIGTERM, SIGQUIT),
	serial({ config.device, O_RDWR }, config.baud),
	context(4),
	pub(context, ZMQ_PUB),
	sub(context, ZMQ_SUB),
	encoder(),
	decoder(config.max_packet_size),
	uart_rx_buf(),
	uart_tx_buf(),
	chunk(CHUNK_SIZE),
	exit(false)
{
	pub.bind(config.rx_url);
	sub.bind(config.tx_url);
	pfds[pfd_signal] = {
		.socket = nullptr,
		.fd = sr.fileno(),
		.events = 0,
		.revents = 0
	};
	pfds[pfd_serial] = {
		.socket = nullptr,
		.fd = serial.fileno(),
		.events = 0,
		.revents = 0
	};
	pfds[pfd_pub_socket] = {
		.socket = &pub,
		.fd = -1,
		.events = 0,
		.revents = 0
	};
	pfds[pfd_sub_socket] = {
		.socket = &sub,
		.fd = -1,
		.events = 0,
		.revents = 0
	};
}

void BridgeTask::run()
{
	pfds[pfd_signal].events = ZMQ_POLLIN;
	pfds[pfd_serial].events = ZMQ_POLLIN | ZMQ_POLLOUT;
	pfds[pfd_pub_socket].events = ZMQ_POLLOUT;
	pfds[pfd_sub_socket].events = ZMQ_POLLIN;
	if (zmq_poll(pfds, pfd_count, -1) == -1) {
		throw Posix::io_error("zmq_poll failed");
	}
	if (pfds[pfd_signal].revents & ZMQ_POLLERR) {
		throw Posix::io_error("zmq_poll failed on signal handler");
	}
	if (pfds[pfd_serial].revents & ZMQ_POLLERR) {
		throw Posix::io_error("zmq_poll failed on serial port");
	}
	if (pfds[pfd_signal].revents & ZMQ_POLLIN) {
		signal_read();
	}
	if (pfds[pfd_sub_socket].revents & ZMQ_POLLIN) {
		sub_socket_read();
	}
	if (pfds[pfd_serial].revents & ZMQ_POLLOUT) {
		serial_write();
	}
	if (pfds[pfd_serial].revents & ZMQ_POLLIN) {
		serial_read();
	}
	if (pfds[pfd_pub_socket].revents & ZMQ_POLLOUT) {
		pub_socket_write();
	}
}

void BridgeTask::run_loop()
{
	while (!exit) {
		run();
	}
}

void BridgeTask::signal_read()
{
	if (sr.read()) {
		exit = true;
	}
}

void BridgeTask::serial_read()
{
	chunk.clear();
	serial.read(chunk);
	std::copy(chunk.cbegin(), chunk.cend(), std::back_inserter(uart_rx_buf));
}

void BridgeTask::serial_write()
{
	chunk.clear();
	std::copy_n(uart_tx_buf.cbegin(), std::min(chunk.capacity(), uart_tx_buf.size()), std::back_inserter(chunk));
	serial.write(chunk);
}

void BridgeTask::pub_socket_write()
{
	auto it = decoder.decode_packet(uart_rx_buf.cbegin(), uart_rx_buf.cend(), Util::GeneratorIterator<const std::vector<std::uint8_t>&>(
		[this] (const auto& packet) {
			zmq::message_t msg(packet.size());
			std::copy(packet.cbegin(), packet.cend(), static_cast<std::uint8_t *>(msg.data()));
			if (!pub.send(msg)) {
				throw Posix::io_error("Failed to publish ØMQ message");
			}
		}));
	uart_rx_buf.erase(uart_rx_buf.cbegin(), it);
}

void BridgeTask::sub_socket_read()
{
	zmq::message_t msg;
	if (!sub.recv(&msg)) {
		throw Posix::io_error("Failed to receive ØMQ message");
	}
	const auto first = static_cast<const std::uint8_t *>(msg.data());
	const auto last = first + msg.size();
	encoder.encode_packet(first, last, std::back_inserter(uart_tx_buf));
}

Bridge::Bridge(const Config& config)
{
	BridgeTask task(config);
	task.run_loop();
}

}
