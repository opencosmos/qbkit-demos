#pragma once
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
