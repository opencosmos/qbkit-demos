#pragma once

/*
 * Wrapper around a pub/sub pair, which "connect" to some remote.
 *
 * This can be used to connect to "tunnel" services such as the serial bridge.
 */

#include <string>
#include <functional>
#include <tuple>
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

struct Envelope
{
	std::string remote;
	std::string session;
	std::string command;
	Envelope() = default;
	Envelope(std::string remote, std::string session, std::string command) :
		remote(std::move(remote)),
		session(std::move(session)),
		command(std::move(command))
			{ }
};

class Socket {
	Logger logger;
	std::string host;
	zmq::socket_t pub;
	zmq::socket_t sub;
public:
	Socket(zmq::context_t& context, const Config& config);

	/* Transmit / receive a command with one data part */
	void send(const Envelope& envelope, zmq::message_t&& data);
	bool recv(Envelope& envelope, zmq::message_t& msg);

	/*
	 * Transmit / receive a command with multiple data parts.
	 *
	 * For recv, sender/command will both be set before the consumer is
	 * called.
	 */
	void send(const Envelope& envelope, std::function<std::pair<zmq::message_t, bool>()> supplier);
	bool recv(Envelope& envelope, std::function<void(zmq::message_t, bool)> consumer);

	zmq::socket_t& get_pub() { return pub; }
	zmq::socket_t& get_sub() { return sub; }
};

}
