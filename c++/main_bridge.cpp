#include <string>
#include <stdexcept>
#include <getopt.h>

#include "Bridge.hpp"

enum args
{
	device,
	baud,
	rx_url,
	tx_url,
	max_packet_size,
	help = 'h',
	invalid = '?'
};

static const struct option long_opts[] = {
	{ "help", no_argument, NULL, help },
	{ "device", required_argument, NULL, device },
	{ "baud", required_argument, NULL, baud },
	{ "rx_url", required_argument, NULL, rx_url },
	{ "tx_url", required_argument, NULL, tx_url },
	{ "max_packet_size", required_argument, NULL, max_packet_size },
	{ NULL, 0, NULL, 0 }
};

static const std::string no_default = "(no default)";

void bridge_default_config(Bridge::Config& config)
{
	config.device = no_default;
	config.baud = 9600;
	config.rx_url = "ipc://serial_rx";
	config.tx_url = "ipc://serial_tx";
	config.max_packet_size = 0x10000;
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
	printf("\t--rx_url=%s\n", config.rx_url.c_str());
	printf("\t--tx_url=%s\n", config.tx_url.c_str());
	printf("\t--max_packet_size=0x%zx\n", config.max_packet_size);
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
	while ((c = getopt_long(argc, argv, "h", long_opts, NULL)) != -1) {
		switch (c) {
		case device: config.device = optarg; break;
		case baud: config.baud = s2i(optarg); break;
		case rx_url: config.tx_url = optarg; break;
		case tx_url: config.rx_url = optarg; break;
		case max_packet_size: config.max_packet_size = s2i(optarg); break;
		case help:
		case invalid:
		default:
			bridge_show_usage(argv0);
			exit(1);
		}
	}
	if (optind != argc || config.device == no_default) {
		bridge_show_usage(argv0);
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	Bridge::Config config;
	bridge_default_config(config);
	bridge_parse_config(config, argc, argv);
	Bridge::Bridge bridge(config);
}
