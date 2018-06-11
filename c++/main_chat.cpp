#include <string>
#include <stdexcept>

#include <getopt.h>

#include <zmq.hpp>

#include "SignalReceiver.hpp"
#include "SignalDispatcher.hpp"
#include "Chat.hpp"

enum args
{
	device,
	baud,
	server_url,
	client_url,
	username,
	help = 'h',
	verbose = 'v',
	invalid = '?'
};

static const struct option long_opts[] = {
	{ "help", no_argument, NULL, help },
	{ "server_url", required_argument, NULL, server_url },
	{ "client_url", required_argument, NULL, client_url },
	{ "username", required_argument, NULL, username },
	{ "verbose", no_argument, NULL, verbose },
	{ NULL, 0, NULL, 0 }
};

static const std::string no_default = "(no default)";

void chat_default_config(Chat::Config& config)
{
	config.host = "chat";
	config.server_url = "ipc:///var/tmp/serial_bridge_server";
	config.client_url = "ipc:///var/tmp/serial_bridge_client";
	config.verbose = false;
}

void chat_show_usage(const char *argv0)
{
	for (const char *c = argv0; *c; c++) {
		if (c[0] == '/' && c[1] != 0) {
			argv0 = c + 1;
		}
	}
	Chat::Config config;
	chat_default_config(config);
	printf("./%s\n", argv0);
	printf("\t--server_url=%s\n", config.server_url.c_str());
	printf("\t--client_url=%s\n", config.client_url.c_str());
}

void chat_parse_config(Chat::Config& config, int argc, char *argv[])
{
	const char *argv0 = argv[0];
	char c;
	while ((c = getopt_long(argc, argv, "hv", long_opts, NULL)) != -1) {
		switch (c) {
		case server_url: config.client_url = optarg; break;
		case client_url: config.server_url = optarg; break;
		case verbose: config.verbose = true; break;
		case username: config.username = optarg; break;
		case help:
		case invalid:
		default:
			chat_show_usage(argv0);
			exit(1);
		}
	}
	if (optind != argc) {
		chat_show_usage(argv0);
		throw std::runtime_error("Unexpected extra arguments");
	}
}

int main(int argc, char *argv[])
{
	Chat::Config config;
	chat_default_config(config);
	chat_parse_config(config, argc, argv);

	Posix::SignalReceiver sr;
	sr.add(SIGQUIT);
	sr.add(SIGINT);
	sr.add(SIGTERM);
	sr.update();

	zmq::context_t ctx(4);

	Posix::SignalDispatcher sd(ctx, sr);

	Chat::Chat chat(ctx, config);

	struct signalfd_siginfo ssi;
	do {
		sd.run(ssi);
	} while (ssi.ssi_signo != SIGINT && ssi.ssi_signo != SIGTERM && ssi.ssi_signo != SIGQUIT);
}
