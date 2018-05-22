#include <stdexcept>

#include <sys/signalfd.h>

#include "SignalReceiver.hpp"

namespace Posix {

static sigset_t& ss_init(sigset_t& ss)
{
	sigemptyset(&ss);
	return ss;
}

SignalReceiver::SignalReceiver() :
	file(::signalfd(-1, &ss_init(ss), SFD_NONBLOCK))
{
}

void SignalReceiver::add(int signo)
{
	::sigaddset(&ss, signo);
}

void SignalReceiver::remove(int signo)
{
	::sigdelset(&ss, signo);
}

void SignalReceiver::set_mask() const
{
	if (::pthread_sigmask(SIG_BLOCK, &ss, NULL) != 0) {
		throw std::runtime_error("pthread_sigmask failed");
	}
}

std::optional<struct ::signalfd_siginfo> SignalReceiver::read()
{
	struct ::signalfd_siginfo ssi;
	auto res = ::read(file.fileno(), &ssi, sizeof(ssi));
	if (res == -1) {
		throw io_error("Failed to read signalfd");
	}
	if (res == 0) {
		return std::nullopt;
	} else {
		return ssi;
	}
}

int SignalReceiver::read_signal()
{
	auto ssi = read();
	if (ssi) {
		return ssi->ssi_signo;
	} else {
		return 0;
	}
}

}
