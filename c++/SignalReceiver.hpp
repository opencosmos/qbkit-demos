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

	template <typename... T>
	void var_add(int signo, T... t)
	{
		add(signo);
		var_add(std::forward<int>(t)...);
	}
	void var_add() {}
public:
	int fileno() const { return file.fileno(); }

	SignalReceiver();

	void add(int signo);
	void remove(int signo);
	void set_mask() const;

	std::optional<struct ::signalfd_siginfo> read();
	int read_signal();

	template <typename... T>
	SignalReceiver(T... t) :
		SignalReceiver()
	{
		var_add(std::forward<int>(t)...);
	}
};

}
