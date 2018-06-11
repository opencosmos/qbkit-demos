#include <string>
#include <stdexcept>

#include <getopt.h>

#include <zmq.hpp>

#include "SignalReceiver.hpp"
#include "SignalDispatcher.hpp"
#include "Bridge.hpp"

enum args
{
	device,
	baud,
	server_url,
	client_url,
	max_packet_size,
	help = 'h',
	verbose = 'v',
	invalid = '?'
};

static const struct option long_opts[] = {
	{ "help", no_argument, NULL, help },
	{ "device", required_argument, NULL, device },
	{ "baud", required_argument, NULL, baud },
	{ "server_url", required_argument, NULL, server_url },
	{ "client_url", required_argument, NULL, client_url },
	{ "max_packet_size", required_argument, NULL, max_packet_size },
	{ "verbose", no_argument, NULL, verbose },
	{ NULL, 0, NULL, 0 }
};

void bridge_default_config(Bridge::Config& config)
{
	config.device = "";
	config.baud = 9600;
	config.server_url = "ipc:///var/tmp/serial_bridge_server";
	config.client_url = "ipc:///var/tmp/serial_bridge_client";
	config.max_packet_size = 0x10000;
	config.verbose = false;
}

void bridge_show_usage(const char *argv0)
{
	for (const char *c = argv0; *c; c++) {
		if (c[0] == '/' && c[1] != 0) {
			argv0 = c + 1;
		}
	}
	Bridge::Config config;
	bridge_default_config(config);
	printf("./%s\n", argv0);
	printf("\t--device=%s\n", config.device.c_str());
	printf("\t--baud=%d\n", config.baud);
	printf("\t--server_url=%s\n", config.server_url.c_str());
	printf("\t--client_url=%s\n", config.client_url.c_str());
	printf("\t--max_packet_size=0x%zx\n", config.max_packet_size);
	printf("\t--verbose\n");
	printf("\n");
	printf("\t%s\n", "If no device is specified then the bridge will operate in loopback mode");
	printf("\n");
}

static long long s2i(const char *s)
{
	char *end;
	long long i = strtoll(s, &end, 0);
	if (*end != 0 || *s == 0) {
		throw std::runtime_error("Invalid integer value: " + std::string(s));
	}
	return i;
}

void bridge_parse_config(Bridge::Config& config, int argc, char *argv[])
{
	const char *argv0 = argv[0];
	char c;
	while ((c = getopt_long(argc, argv, "hv", long_opts, NULL)) != -1) {
		switch (c) {
		case device: config.device = optarg; break;
		case baud: config.baud = s2i(optarg); break;
		case server_url: config.client_url = optarg; break;
		case client_url: config.server_url = optarg; break;
		case max_packet_size: config.max_packet_size = s2i(optarg); break;
		case verbose: config.verbose = true; break;
		case help:
		case invalid:
		default:
			bridge_show_usage(argv0);
			exit(1);
		}
	}
	if (optind != argc) {
		bridge_show_usage(argv0);
		throw std::runtime_error("Unexpected extra arguments");
	}
}

int main(int argc, char *argv[])
{
	Bridge::Config config;
	bridge_default_config(config);
	bridge_parse_config(config, argc, argv);

	Posix::SignalReceiver sr;
	sr.add(SIGQUIT);
	sr.add(SIGINT);
	sr.add(SIGTERM);
	sr.update();

	zmq::context_t ctx(4);

	Posix::SignalDispatcher sd(ctx, sr);

	Bridge::Bridge bridge(config, ctx);

	struct signalfd_siginfo ssi;
	do {
		sd.run(ssi);
	} while (ssi.ssi_signo != SIGINT && ssi.ssi_signo != SIGTERM && ssi.ssi_signo != SIGQUIT);
}
