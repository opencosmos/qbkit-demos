#include "SystemError.hpp"

#include "Protocol.hpp"

namespace Comms {

static constexpr char label_delimiter = '.';

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

static bool do_send(zmq::socket_t& socket, zmq::message_t& msg, bool more)
{
	return socket.send(msg, more ? ZMQ_SNDMORE : 0);
}

static bool do_recv(zmq::socket_t& socket, zmq::message_t& msg, bool expect_more)
{
	return socket.recv(&msg) && socket.getsockopt<int>(ZMQ_RCVMORE) == expect_more;
}

static bool do_send_label(zmq::socket_t& socket, const std::string& label)
{
	zmq::message_t msg(label.length() + 1);
	std::memcpy(msg.data(), label.data(), label.length());
	static_cast<char *>(msg.data())[label.length()] = label_delimiter;
	return do_send(socket, msg, true);
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

void Socket::send(const std::string& dest, const std::string& command, zmq::message_t& data)
{
	logger("Sending \"", command, "\" to \"", dest, "\" with ", data.size(), " bytes of data");
	if (!(do_send_label(pub, dest) &&
			do_send_label(pub, host) &&
			do_send_label(pub, command) &&
			do_send(pub, data, false))) {
		throw SystemError("Failed to send message");
	}
}

bool Socket::recv(std::string& sender, std::string& command, zmq::message_t& data)
{
	std::string target;
	if (!(do_recv_label(sub, target) &&
			do_recv_label(sub, sender) &&
			do_recv_label(sub, command) &&
			do_recv(sub, data, false) &&
			target == host)) {
		do_consume(sub);
		return false;
	}
	logger("Received \"", command, "\" to \"", sender, "\" with ", data.size(), " bytes of data");
	return true;
}

}
