#pragma once

/*
 * Receives signals via SignalReceiver and dispatches the signalfd_siginfo
 * structures via ZeroMQ to other threads.
 *
 * Allows you to use a single thread for receiving certain signals, and then to
 * broadcast them to other threads which have been configured to block those
 * signals.
 *
 * This way, either all threads receive the signal, or none of them do (the
 * signal can't just go to one thread but not others).
 */

#include <string>

#include <zmq.hpp>

#include <sys/signalfd.h>

#include "SignalReceiver.hpp"

namespace Posix {

class SignalDispatcher {
	zmq::socket_t signal_pub;
	const SignalReceiver& sr;
public:
	static constexpr char url[] = "inproc://signal";
	SignalDispatcher(zmq::context_t& context, const SignalReceiver& sr);
	void run(struct signalfd_siginfo& ssi);
};

}
