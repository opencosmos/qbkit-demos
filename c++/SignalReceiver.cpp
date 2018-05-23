#include <stdexcept>

#include <unistd.h>
#include <sys/signalfd.h>

#include "SystemError.hpp"

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

void SignalReceiver::update() const
{
	if (::pthread_sigmask(SIG_BLOCK, &ss, NULL) != 0) {
		throw SystemError("pthread_sigmask failed");
	}
	if (::signalfd(fileno(), &ss, SFD_NONBLOCK) == -1) {
		throw SystemError("signalfd failed");
	}
}

std::optional<struct ::signalfd_siginfo> SignalReceiver::read() const
{
	struct ::signalfd_siginfo ssi;
	auto res = ::read(file.fileno(), &ssi, sizeof(ssi));
	if (res == -1) {
		throw SystemError("Failed to read signalfd");
	}
	if (res == 0) {
		return std::nullopt;
	} else {
		return ssi;
	}
}

int SignalReceiver::read_signal() const
{
	auto ssi = read();
	if (ssi) {
		return ssi->ssi_signo;
	} else {
		return 0;
	}
}

}
