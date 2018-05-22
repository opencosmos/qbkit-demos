#pragma once

/*
 * Uses signalfd to receive signals.
 *
 * When update() is called then the signal descriptor is updated and the
 * pthread_sigmask is also called in order to block the selected signals.
 *
 * Known Bugs:
 *  - If a signal is later removed and then update() is called, the signal is
 *    removed from the signalfd but is not unblocked.
 */

#include <optional>

#include <signal.h>
#include <sys/signalfd.h>

#include "File.hpp"

namespace Posix {

class SignalReceiver
{
	::sigset_t ss;
	File file;
public:
	int fileno() const { return file.fileno(); }

	SignalReceiver();

	void add(int signo);
	void remove(int signo);
	void update() const;

	std::optional<struct ::signalfd_siginfo> read() const;
	int read_signal() const;
};

}
