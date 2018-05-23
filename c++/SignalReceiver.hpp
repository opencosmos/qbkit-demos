#pragma once
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
