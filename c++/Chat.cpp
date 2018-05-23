#include <deque>
#include <vector>
#include <string>
#include <thread>
#include <cstdint>

#include <unistd.h>

#include <zmq.hpp>

#include "SystemError.hpp"
#include "Protocol.hpp"
#include "File.hpp"
#include "Task.hpp"

#include "Chat.hpp"

namespace Chat {

class Task :
	public Util::Task
{
	Config config;
	Comms::Socket socket;
	Posix::File in;

	const int pfd_stdin;
	const int pfd_sub;

	/* STDIN line buffer */
	std::deque<char> line;

	/* General-purpose I/O buffer */
	std::vector<std::uint8_t> buf;

	bool eof = false;

	void stdin_read();
	void sub_read();
	void flush_lines();

protected:
	virtual void set_events() override;
	virtual void handle_events() override;

public:
	Task(zmq::context_t& ctx, const Config& config);
};

Task::Task(zmq::context_t& ctx, const Config& config) :
	Util::Task(ctx, config.host, config.verbose),
	config(config),
	socket(ctx, config),
	in(STDIN_FILENO),
	pfd_stdin(bind(STDIN_FILENO)),
	pfd_sub(bind(socket.get_sub()))
{
	buf.reserve(200);
	logger("Using username \"", config.username, "\"");
}

void Task::stdin_read()
{
	if (in.read(buf)) {
		std::copy(buf.cbegin(), buf.cend(), std::back_inserter(line));
	} else {
		eof = true;
		if (!line.empty()) {
			line.push_back('\n');
		}
	}
}

void Task::sub_read()
{
	std::string sender;
	std::string username;
	zmq::message_t msg;
	if (socket.recv(sender, username, msg)) {
		std::string message(static_cast<const char *>(msg.data()), msg.size());
		std::cout << "[" << username << "] " << message << std::endl;
	}
}

void Task::flush_lines()
{
	/*
	 * Flush complete lines from buffer to STDOUT.
	 *
	 * Flush all remaining data to STDOUT if end of input has been reached.
	 */
	while (!line.empty()) {
		auto eol = eof ? line.cend() - 1 : std::find(line.cbegin(), line.cend(), '\n');
		if (eol == line.cend()) {
			break;
		}
		buf.clear();
		std::copy(line.cbegin(), eol, std::back_inserter(buf));
		line.erase(line.cbegin(), eol + 1);
		zmq::message_t msg(buf.size());
		std::memcpy(msg.data(), buf.data(), buf.size());
		socket.send(config.host, config.username, msg);
	}
}

void Task::set_events()
{
	get_pfd(pfd_stdin).events = eof ? 0 : ZMQ_POLLIN;
	get_pfd(pfd_sub).events = ZMQ_POLLIN;
}

void Task::handle_events()
{
	/* Read STDIN into buffer */
	if (get_pfd(pfd_stdin).revents & ZMQ_POLLIN) {
		stdin_read();
	}
	/* Write received message to STDOUT */
	if (get_pfd(pfd_sub).revents & ZMQ_POLLIN) {
		sub_read();
	}
	/* Write out lines */
	flush_lines();
	/* If all data flushed and no more input, exit */
	if (eof && line.empty()) {
		exit();
	}
}

Chat::Chat(zmq::context_t& ctx, const Config& config) :
	worker([&] () {
		Task task(ctx, config);
		task.run_loop();
	})
{
}

Chat::~Chat()
{
	worker.join();
}

}
