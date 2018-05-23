#include "SignalDispatcher.hpp"
#include "SystemError.hpp"

#include "Task.hpp"

namespace Util {

int Task::bind(int fd)
{
	const auto i = pfds.size();
	auto& pfd = pfds.emplace_back();
	pfd.socket = nullptr;
	pfd.fd = fd;
	pfd.events = 0;
	pfd.revents = 0;
	return i;
}

int Task::bind(zmq::socket_t& socket)
{
	const auto i = pfds.size();
	auto& pfd = pfds.emplace_back();
	pfd.socket = socket;
	pfd.fd = -1;
	pfd.events = 0;
	pfd.revents = 0;
	return i;
}

zmq_pollitem_t& Task::get_pfd(int handle)
{
	return pfds[handle];
}

void Task::run()
{
	/* Bind event handlers */
	get_pfd(pfd_signal).events = ZMQ_POLLIN;
	set_events();
	/* Poll */
	if (zmq_poll(pfds.data(), pfds.size(), -1) == -1) {
		throw SystemError("zmq_poll failed");
	}
	/* Call event handlers */
	if (pfds[pfd_signal].revents & ZMQ_POLLERR) {
		throw SystemError("zmq_poll failed on signal handler");
	}
	if (pfds[pfd_signal].revents & ZMQ_POLLIN) {
		signal_read();
	}
	handle_events();
}

void Task::exit()
{
	exiting = true;
}

void Task::signal_read()
{
	zmq::message_t msg;
	if (!signal_sub.recv(&msg)) {
		throw SystemError("Failed to receive ØMQ message");
	}
	struct signalfd_siginfo ssi;
	if (msg.size() != sizeof(ssi)) {
		throw std::runtime_error("Unknown data type");
	}
	std::memcpy(&ssi, msg.data(), sizeof(ssi));
	if (ssi.ssi_signo == SIGINT || ssi.ssi_signo == SIGQUIT || ssi.ssi_signo == SIGTERM) {
		logger("Caught signal ", strsignal(ssi.ssi_signo), ", exiting");
		exiting = true;
	}
}

Task::Task(zmq::context_t& context, const std::string& log_name, bool verbose) :
	signal_sub(context, ZMQ_SUB),
	pfd_signal(bind(signal_sub)),
	logger(log_name, verbose)
{
	signal_sub.connect(Posix::SignalDispatcher::url);
	signal_sub.setsockopt(ZMQ_SUBSCRIBE, "", 0);
}

void Task::run_loop()
{
	while (!exiting) {
		run();
	}
}

}
