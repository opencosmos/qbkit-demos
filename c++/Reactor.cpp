#include "SignalDispatcher.hpp"
#include "SystemError.hpp"

#include "Reactor.hpp"

namespace Util {

void Reactor::bind_read(pfd_key fd)
{
	pfd_read.emplace(fd);
}

void Reactor::bind_write(pfd_key fd)
{
	pfd_write.emplace(fd);
}

void Reactor::bind_error(pfd_key fd)
{
	pfd_error.emplace(fd);
}

bool Reactor::is_readable(pfd_key fd) const
{
	return pfd_read.find(fd) != pfd_read.cend();
}

bool Reactor::is_writeable(pfd_key fd) const
{
	return pfd_write.find(fd) != pfd_write.cend();
}

bool Reactor::is_erroring(pfd_key fd) const
{
	return pfd_error.find(fd) != pfd_error.cend();
}

const struct signalfd_siginfo& Reactor::get_signal() const
{
	return ssi;
}

bool Reactor::run()
{
	/*
	 * Intended to be easy to use, not efficient.
	 *
	 * This is a demo program after all.
	 */
	pfd_read.clear();
	pfd_write.clear();
	pfd_error.clear();
	std::memset(&ssi, 0, sizeof(ssi));
	/* Bind event handlers */
	bind_read(signal_sub);
	bind_error(signal_sub);
	set_events();
	/* Build list of pollitem_t structures */
	const auto max_pfd = pfd_read.size() + pfd_write.size();
	std::vector<zmq::pollitem_t> p;
	p.reserve(max_pfd);
	std::unordered_map<pfd_key, zmq::pollitem_t *> xp(max_pfd);
	const auto get_pfd = [&] (const pfd_key key) -> zmq::pollitem_t& {
		auto [it, is_new] = xp.emplace(key, nullptr);
		auto or_else = [] (auto *value, auto def) {
			return value == nullptr ? def : *value;
		};
		if (is_new) {
			auto& pfd = p.emplace_back();
			it->second = &pfd;
			pfd.fd = or_else(std::get_if<int>(&key), -1);
			pfd.socket = or_else(std::get_if<socket_ref>(&key), nullptr);
			pfd.events = 0;
			pfd.revents = 0;
		}
		return *it->second;
	};
	for (const auto& key : pfd_read) {
		get_pfd(key).events |= ZMQ_POLLIN;
	}
	for (const auto& key : pfd_write) {
		get_pfd(key).events |= ZMQ_POLLOUT;
	}
	for (const auto& key : pfd_error) {
		/* Ignored by poll/zmq_poll, used when checking afterward */
		get_pfd(key).events |= ZMQ_POLLERR;
	}
	pfd_read.clear();
	pfd_write.clear();
	pfd_error.clear();
	/* Poll */
	if (zmq_poll(p.data(), p.size(), -1) == -1) {
		throw SystemError("zmq_poll failed");
	}
	for (const auto& [key, value] : xp) {
		if (value->revents & ZMQ_POLLIN) {
			pfd_read.emplace(key);
		}
		if (value->revents & ZMQ_POLLOUT) {
			pfd_write.emplace(key);
		}
		if (value->revents & ZMQ_POLLERR && !(value->revents & ZMQ_POLLERR)) {
			errno = 0;
			throw SystemError("Error on descriptor/socket with no error-handler bound");
		}
	}
	/* Call event handlers */
	if (is_readable(signal_sub)) {
		signal_read();
	}
	handle_events();
	/*
	 * Return whether there were events bound besides the built-in signal
	 * listener
	 */
	return p.size() > 1;
}

void Reactor::terminate()
{
	terminating = true;
}

void Reactor::exit()
{
	exiting = true;
}

void Reactor::signal_read()
{
	zmq::message_t msg;
	if (!signal_sub.recv(&msg)) {
		throw SystemError("Failed to receive ØMQ message");
	}
	if (msg.size() != sizeof(ssi)) {
		throw std::runtime_error("Unknown data type");
	}
	std::memcpy(&ssi, msg.data(), sizeof(ssi));
	if (ssi.ssi_signo == SIGINT || ssi.ssi_signo == SIGQUIT || ssi.ssi_signo == SIGTERM) {
		logger("Caught signal ", strsignal(ssi.ssi_signo), ", exiting");
		if (ssi.ssi_signo == SIGQUIT) {
			exit();
		} else {
			terminate();
		}
	}
}

bool Reactor::run_loop()
{
	while (true) {
		if (terminating) {
			logger("Terminating event-loop");
			return false;
		}
		if (!run() && exiting) {
			logger("Exiting event-loop");
			return true;
		}
	}
}

Reactor::Reactor(zmq::context_t& context, const std::string& log_name, bool verbose) :
	signal_sub(context, ZMQ_SUB),
	logger(log_name, verbose)
{
	signal_sub.connect(Posix::SignalDispatcher::url);
	signal_sub.setsockopt(ZMQ_SUBSCRIBE, "", 0);
}

}
