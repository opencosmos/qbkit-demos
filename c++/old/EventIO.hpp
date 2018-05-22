#pragma once
#include <functional>
#include <unordered_set>

#include "File.hpp"

namespace Posix {

struct EventHandler
{
	File& file;

	std::function<void()> read;
	std::function<void()> write;
	std::function<void()> error;

	EventHandler(File& file) :
		file(file)
	{
	}

	EventHandler& on_read(std::function<void()> handler)
	{
		read = std::move(handler);
		return *this;
	}

	EventHandler& on_write(std::function<void()> handler)
	{
		write = std::move(handler);
		return *this;
	}

	EventHandler& on_error(std::function<void()> handler)
	{
		error = std::move(handler);
		return *this;
	}
};

}

template <>
class std::hash<Posix::EventHandler>
{
	int operator () (const Posix::EventHandler& eh) const
	{
		return eh.file.fileno();
	}
};

namespace Posix {

class EventIO
{
	std::unordered_set<EventHandler> handlers;
public:
	void add(EventHandler eh)
	{
		handlers.emplace(std::move(eh));
	}
	void remove(const EventHandler& eh)
	{
		handlers.erase(eh);
	}
	void run()
	{
		/* TODO */
	}
};

}
