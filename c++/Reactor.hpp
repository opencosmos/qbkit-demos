#pragma once

/*
 * Wrapper for tasks which use the reactor pattern.
 *
 * Uses zmq_poll for the event loop.
 *
 * Subscribes to the SignalDispatcher, in order to receive INT/QUIT/TERM signals
 * (all of which will result in exiting the event loop).
 */

#include <unordered_set>
#include <variant>

#include <sys/signalfd.h>

#include <zmq.hpp>

#include "Logger.hpp"

namespace Util {

/*
 * Can be terminated via signals broadcast via the inproc signal distributor.
 */
class Reactor
{
	using socket_ref = decltype(zmq::pollitem_t{}.socket);
	using pfd_key = std::variant<int, socket_ref>;
	std::unordered_set<pfd_key> pfd_read;
	std::unordered_set<pfd_key> pfd_write;
	std::unordered_set<pfd_key> pfd_error;

	struct signalfd_siginfo ssi;

	/* Exit requested? */
	bool exiting = false;
	bool terminating = false;

	/* Signal subscriber */
	zmq::socket_t signal_sub;

	/* Handles received signals */
	void signal_read();

protected:
	const Logger logger;

	void bind_read(pfd_key fd);
	void bind_write(pfd_key fd);
	void bind_error(pfd_key fd);

	bool is_readable(pfd_key fd) const;
	bool is_writeable(pfd_key fd) const;
	bool is_erroring(pfd_key fd) const;

	const struct signalfd_siginfo& get_signal() const;

	/* Called in run() before zmq_poll to set the "events" fields */
	virtual void set_events() = 0;
	/* Called in run() after zmq_poll to test the "revents" fields */
	virtual void handle_events() = 0;

	/* Runs an iteration of the event-loop */
	bool run();

public:
	/* Call to request shutdown of event-loop */
	void exit();

	/* Call to request exit of event-loop */
	void terminate();

	/* Construct using the given ØMQ context, logger name and verbosity */
	Reactor(zmq::context_t& context, const std::string& log_name, bool verbose);

	/* Start the event-loop */
	bool run_loop();
};

}
