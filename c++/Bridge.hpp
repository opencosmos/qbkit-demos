#pragma once
#include <string>
#include <cstddef>

#include "Kiss.hpp"

namespace Bridge {

struct Config
{
	std::string device;
	int baud;
	std::string tx_url;
	std::string rx_url;
	std::size_t max_packet_size;
	bool quiet;
	Kiss::Config kiss_config;
};

class Bridge
{
public:
	Bridge(const Config& config);
};

}
