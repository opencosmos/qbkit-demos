#pragma once

/*
 * Wrapper for tasks which use the reactor pattern.
 *
 * Uses zmq_poll for the event loop.
 *
 * Subscribes to the SignalDispatcher, in order to receive INT/QUIT/TERM signals
 * (all of which will result in exiting the event loop).
 */

#include <vector>

#include <zmq.hpp>

#include "Logger.hpp"

namespace Util {

/*
 * Base class for tasks which run a zmq_poll event-loop.
 *
 * Can be interrupted via signals broadcast via the inproc signal distributor.
 */
class Task
{
	std::vector<zmq_pollitem_t> pfds;

	/* Exit requested? */
	bool exiting;

	/* Signal subscriber */
	zmq::socket_t signal_sub;

	/* Handle to signalfd binding */
	const int pfd_signal;

	/* Handles received signals */
	void signal_read();

protected:
	const Logger logger;

	/* Call to bind a file-descriptor to the event-loop, and get a handle */
	int bind(int fd);
	/* Call to bind a ØMQ socket to the event-loop, and get a handle */
	int bind(zmq::socket_t& socket);

	/*
	 * Call to get the zmq_pollitem_t struct for the given handle.
	 *
	 * Note: the reference is invalidated by calls to bind.
	 */
	zmq_pollitem_t& get_pfd(int handle);

	/* Called in run() before zmq_poll to set the "events" fields */
	virtual void set_events() = 0;
	/* Called in run() after zmq_poll to test the "revents" fields */
	virtual void handle_events() = 0;

	/* Runs an iteration of the event-loop */
	void run();

	/* Call to request exit of event-loop */
	void exit();

public:
	/* Construct using the given ØMQ context, logger name and verbosity */
	Task(zmq::context_t& context, const std::string& log_name, bool verbose);

	/* Start the event-loop */
	void run_loop();
};

}
