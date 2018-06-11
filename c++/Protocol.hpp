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

struct SocketConfig
{
	std::string host;
	std::string url;
	bool verbose;
};

struct ServerSocketConfig :
	SocketConfig
{
};

struct ClientSocketConfig :
	SocketConfig
{
	float timeout; /* seconds */
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
protected:
	zmq::socket_t socket;
	Socket(zmq::context_t& context, int type, const SocketConfig& config);
	virtual void pre_connect();

public:
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

	zmq::socket_t& get_socket() { return socket; }
};

class ClientSocket : public Socket {
	float timeout;
	virtual void pre_connect() override;
public:
	ClientSocket(zmq::context_t& context, const ClientSocketConfig& config) :
		Socket(context, ZMQ_REQ, config),
		timeout(config.timeout)
	{ }
};

class ServerSocket : public Socket {
public:
	ServerSocket(zmq::context_t& context, const ServerSocketConfig& config) :
		Socket(context, ZMQ_REP, config)
	{ }
};

}
