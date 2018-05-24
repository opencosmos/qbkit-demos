#include <iostream>

#include "SystemError.hpp"

#include "Protocol.hpp"

namespace Comms {

static constexpr bool log_packets = false;

static constexpr char label_delimiter = '\0';

Socket::Socket(zmq::context_t& context, const Config& config) :
	logger(config.host, config.verbose),
	host(config.host),
	pub(context, ZMQ_PUB),
	sub(context, ZMQ_SUB)
{
	pub.setsockopt(ZMQ_LINGER, 0);
	sub.setsockopt(ZMQ_LINGER, 0);

	pub.connect(config.tx_url);
	sub.connect(config.rx_url);

	sub.setsockopt(ZMQ_SUBSCRIBE, (host + label_delimiter).c_str(), 0);
}

static std::pair<bool, bool> do_recv(zmq::socket_t& socket, zmq::message_t& msg)
{
	bool success = socket.recv(&msg);
	bool more = success && socket.getsockopt<int>(ZMQ_RCVMORE);
	if constexpr (log_packets) {
		std::cerr << "<< " << std::string((char *) msg.data(), msg.size()) << std::endl;
		if (!more) {
			std::cerr << "--" << std::endl;
		}
	}
	return { success, more };
}

static bool do_recv(zmq::socket_t& socket, zmq::message_t& msg, bool expect_more)
{
	bool success;
	bool more;
	std::tie(success, more) = do_recv(socket, msg);
	return success && more == expect_more;
}

static bool do_recv_label(zmq::socket_t& socket, std::string& label)
{
	zmq::message_t msg;
	do_recv(socket, msg, true);
	if (msg.size() == 0) {
		return false;
	}
	if (static_cast<char *>(msg.data())[msg.size() - 1] != label_delimiter) {
		return false;
	}
	label = { static_cast<char *>(msg.data()), msg.size() - 1 };
	return true;
}

static void do_consume(zmq::socket_t& socket)
{
	while (socket.getsockopt<int>(ZMQ_RCVMORE)) {
		zmq::message_t msg;
		if (!socket.recv(&msg)) {
			throw SystemError("Failed to receive packet");
		}
	}
}

static bool do_send(zmq::socket_t& socket, zmq::message_t&& msg, bool more)
{
	if constexpr (log_packets) {
		std::cerr << ">> " << std::string((char *) msg.data(), msg.size()) << std::endl;
		if (!more) {
			std::cerr << "--" << std::endl;
		}
	}
	return socket.send(std::move(msg), more ? ZMQ_SNDMORE : 0);
}

static bool do_send_label(zmq::socket_t& socket, const std::string& label)
{
	zmq::message_t msg(label.length() + 1);
	std::memcpy(msg.data(), label.data(), label.length());
	static_cast<char *>(msg.data())[label.length()] = label_delimiter;
	return do_send(socket, std::move(msg), true);
}

void Socket::send(const Envelope& envelope, zmq::message_t&& data)
{
	send(envelope, [&] () {
		return std::make_pair(std::move(data), false);
	});
}

bool Socket::recv(Envelope& envelope, zmq::message_t& data)
{
	return recv(envelope, [&] (zmq::message_t msg, bool more) {
		data = std::move(msg);
		if (more) {
			errno = EBADMSG;
			throw SystemError("Message has more parts than expected");
		}
	});
}

void Socket::send(const Envelope& envelope, std::function<std::pair<zmq::message_t, bool>()> supplier)
{
	logger("Sending stream with command \"", envelope.command, "\" to session \"", envelope.session, "\" on \"", envelope.remote, "\"");
	bool success = true;
	success = success && do_send_label(pub, envelope.remote);
	success = success && do_send_label(pub, host);
	success = success && do_send_label(pub, envelope.session);
	success = success && do_send_label(pub, envelope.command);
	bool more = true;
	while (success && more) {
		zmq::message_t data;
		std::tie(data, more) = supplier();
		const auto size = data.size();
		success = do_send(pub, std::move(data), more);
		if (success) {
			logger("Sent part with ", size, " bytes of data");
		}
	}
	if (!success) {
		logger("Failed");
		throw SystemError("Failed to send message");
	}
}

bool Socket::recv(Envelope& envelope, std::function<void(zmq::message_t, bool)> consumer)
{
	logger("Receiving stream");
	bool success = true;
	std::string target;
	success = success && do_recv_label(sub, target);
	if (target != host) {
		logger("Stream is addressed to host \"", target, "\" not \"", host, "\"");
		do_consume(sub);
		return false;
	}
	success = success && do_recv_label(sub, envelope.remote);
	success = success && do_recv_label(sub, envelope.session);
	success = success && do_recv_label(sub, envelope.command);
	logger("Receiving stream with command \"", envelope.command, "\" from session \"", envelope.session, "\" for \"", envelope.remote, "\"");
	bool more = true;
	while (success && more) {
		zmq::message_t data;
		std::tie(success, more) = do_recv(sub, data);
		auto size = data.size();
		consumer(std::move(data), more);
		if (success) {
			logger("Received part with ", size, " bytes of data");
		}
	}
	if (!success) {
		logger("Failed");
		do_consume(sub);
		return false;
	}
	return true;
}

}
