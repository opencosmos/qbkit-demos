#include "Logger.hpp"

std::mutex Logger::mx;
int Logger::next_id;

int Logger::get_id()
{
	std::lock_guard<std::mutex> lock(mx);
	return next_id++;
}
