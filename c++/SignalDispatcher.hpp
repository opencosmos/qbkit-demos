#pragma once
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
