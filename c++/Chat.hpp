#pragma once

/*
 * Example instant-messaging program for use with the serial bridge.
 *
 * This uses multi-part messages.
 *
 * The "session" field is used to transmit a sender's "screen name" with their
 * message.
 */

#include <string>
#include <thread>

#include <zmq.hpp>

#include "Protocol.hpp"

namespace Chat {

struct Config :
	Comms::Config
{
	std::string username;
};

class Chat
{
	std::thread worker;

public:
	Chat(zmq::context_t& ctx, const Config& config);
	~Chat();
};

}
