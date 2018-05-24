#pragma once

/*
 * Wrapper around a pub/sub pair, which "connect" to some remote.
 *
 * This can be used to connect to "tunnel" services such as the serial bridge.
 */

#include <string>
#include <cstddef>

#include <zmq.hpp>

#include "Logger.hpp"

namespace Comms {

struct Config {
	std::string host;
	std::string rx_url;
	std::string tx_url;
	bool verbose;
};

class Socket {
	Logger logger;
	std::string host;
	zmq::socket_t pub;
	zmq::socket_t sub;
public:
	Socket(zmq::context_t& context, const Config& config);

	void send(const std::string& dest, const std::string& command, zmq::message_t& data);
	bool recv(std::string& sender, std::string& command, zmq::message_t& msg);

	zmq::socket_t& get_pub() { return pub; }
	zmq::socket_t& get_sub() { return sub; }
};

}
