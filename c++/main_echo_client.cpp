#include <string>
#include <stdexcept>
#include <thread>
#include <atomic>
#include <iostream>

#include <getopt.h>

#include <zmq.hpp>

#include "SignalReceiver.hpp"
#include "SignalDispatcher.hpp"

#include "Protocol.hpp"

enum args
{
	client_url,
	help = 'h',
	verbose = 'v',
	invalid = '?'
};

static const struct option long_opts[] = {
	{ "help", no_argument, NULL, help },
	{ "client_url", required_argument, NULL, client_url },
	{ "verbose", no_argument, NULL, verbose },
	{ NULL, 0, NULL, 0 }
};

static const std::string no_default = "(no default)";

void echo_default_config(Comms::ClientSocketConfig& config)
{
	config.host = "chat";
	config.url = "ipc:///var/tmp/serial_bridge_client";
	config.verbose = false;
}

void echo_show_usage(const char *argv0)
{
	for (const char *c = argv0; *c; c++) {
		if (c[0] == '/' && c[1] != 0) {
			argv0 = c + 1;
		}
	}
	Comms::ClientSocketConfig config;
	echo_default_config(config);
	printf("./%s\n", argv0);
	printf("\t--client_url=%s\n", config.url.c_str());
}

void echo_parse_config(Comms::ClientSocketConfig& config, int argc, char *argv[])
{
	const char *argv0 = argv[0];
	char c;
	while ((c = getopt_long(argc, argv, "hv", long_opts, NULL)) != -1) {
		switch (c) {
		case client_url: config.url = optarg; break;
		case verbose: config.verbose = true; break;
		case help:
		case invalid:
		default:
			echo_show_usage(argv0);
			exit(1);
		}
	}
	if (optind != argc) {
		echo_show_usage(argv0);
		throw std::runtime_error("Unexpected extra arguments");
	}
}

int main(int argc, char *argv[])
{
	Comms::ClientSocketConfig config;
	echo_default_config(config);
	echo_parse_config(config, argc, argv);

	Posix::SignalReceiver sr;
	sr.add(SIGQUIT);
	sr.add(SIGINT);
	sr.add(SIGTERM);
	sr.update();

	zmq::context_t ctx(4);

	Posix::SignalDispatcher sd(ctx, sr);

	std::atomic<bool> exiting(false);

	std::thread input_thread([&] () {
		using namespace Comms;
		using namespace std;
		using namespace zmq;
		ClientSocket socket(ctx, config);
		while (exiting) {
			string line;
			getline(cin, line);
			Envelope envelope;
			message_t req(line.size());
			copy(line.cbegin(), line.cend(), static_cast<char *>(req.data()));
			socket.send(envelope, std::move(req));
		}
	});

	std::thread output_thread([&] () {
	});

	struct signalfd_siginfo ssi;
	do {
		sd.run(ssi);
	} while (ssi.ssi_signo != SIGINT && ssi.ssi_signo != SIGTERM && ssi.ssi_signo != SIGQUIT);

	exiting = true;

	input_thread.join();
	output_thread.join();
}
