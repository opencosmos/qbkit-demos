#pragma once

/*
 * Serial/ZMQ bridge implementation.
 *
 * Preserves multi-part messages over the serial link.
 */

#include <string>
#include <cstddef>
#include <thread>

#include <zmq.hpp>

#include "Kiss.hpp"
#include "Protocol.hpp"

namespace Bridge {

struct Config :
	Comms::Config
{
	std::string device;
	int baud;
	std::size_t max_packet_size;

	Kiss::Config kiss_config;
};

class Bridge
{
	std::thread worker;
public:
	Bridge(const Config& config, zmq::context_t& ctx);
	~Bridge();
};

}
