#pragma once

/*
 * Simple logger, which writes to stderr.
 *
 * Uses global mutex in order to prevent message mixing on stderr.
 */

#include <mutex>
#include <string>
#include <sstream>
#include <iostream>

class Logger
{
	static std::mutex mx;
	template <typename Arg, typename... Args>
	static void log(std::ostringstream& oss, Arg&& arg, Args&&... args)
	{
		oss << arg;
		log(oss, std::forward<Args>(args)...);
	}
	static void log(std::ostringstream& oss)
	{
		(void) oss;
	}

	std::string name;
	bool enabled;
public:
	Logger(std::string name, bool enabled = false) :
		name(std::move(name)),
		enabled(enabled)	{ }
	template <typename... Args>
	void operator () (Args&&... args) const
	{
		std::lock_guard<std::mutex> lock(mx);
		if (!enabled) {
			return;
		}
		std::ostringstream oss;
		log(oss, std::forward<Args>(args)...);
		std::cerr << "  [" << name << "] " << oss.str() << std::endl;
	}
};
