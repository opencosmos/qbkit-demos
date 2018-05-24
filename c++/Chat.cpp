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
#include "Reactor.hpp"

#include "Chat.hpp"

namespace Chat {

static constexpr char msg_command[] = "message";

class Task :
	public Util::Reactor
{
	Config config;
	Comms::Socket socket;
	Posix::File in;

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
	Util::Reactor(ctx, config.host, config.verbose),
	config(config),
	socket(ctx, config),
	in(STDIN_FILENO)
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
	Comms::Envelope envelope;
	zmq::message_t msg;
	if (socket.recv(envelope, msg)) {
		if (envelope.command == msg_command) {
			std::string message(static_cast<const char *>(msg.data()), msg.size());
			std::cout << "[" << envelope.session << "] " << message << std::endl;
		} else {
			std::cout << "[" << envelope.session << "] unknown message type: \"" << envelope.command << std::endl;
		}
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
		socket.send({ config.host, config.username, msg_command }, std::move(msg));
	}
}

void Task::set_events()
{
	if (!eof) {
		bind_read(STDIN_FILENO);
	}
	bind_read(socket.get_sub());
}

void Task::handle_events()
{
	/* Read STDIN into buffer */
	if (is_readable(STDIN_FILENO)) {
		stdin_read();
	}
	/* Write received message to STDOUT */
	if (is_readable(socket.get_sub())) {
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
		Task reactor(ctx, config);
		reactor.run_loop();
	})
{
}

Chat::~Chat()
{
	worker.join();
}

}
