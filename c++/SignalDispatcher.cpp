#include <poll.h>

#include "SystemError.hpp"

#include "SignalDispatcher.hpp"

namespace Posix {

SignalDispatcher::SignalDispatcher(zmq::context_t& context, const SignalReceiver& sr) :
	signal_pub(context, ZMQ_PUB),
	sr(sr)
{
	signal_pub.bind(url);
}

void SignalDispatcher::run(struct signalfd_siginfo& ssi)
{
	struct pollfd pfd = {
		.fd = sr.fileno(),
		.events = POLLIN,
		.revents = 0
	};
	while (!(pfd.revents & POLLIN)) {
		int ret = poll(&pfd, 1, -1);
		if (ret == -1) {
			throw SystemError("poll failed");
		}
	}
	ssi = *sr.read();
	zmq::message_t msg(sizeof(ssi));
	std::memcpy(msg.data(), &ssi, sizeof(ssi));
	if (!signal_pub.send(msg)) {
		throw SystemError("zmq_send failed");
	}
}

}
