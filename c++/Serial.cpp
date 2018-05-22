#include <unordered_map>

#include <termios.h>

#include "Serial.hpp"

#define X(b) { b, B##b }
static const std::unordered_map<int, int> baud_constants = {
	X(50),
	X(75),
	X(110),
	X(134),
	X(150),
	X(200),
	X(300),
	X(600),
	X(1200),
	X(1800),
	X(2400),
	X(4800),
	X(9600),
	X(19200),
	X(38400),
	X(57600),
	X(115200),
	X(230400),
	X(460800),
	X(500000),
	X(921600),
	X(1000000),
	X(1152000),
	X(1500000),
	X(2000000),
	X(2500000),
	X(3000000),
	X(3500000),
	X(4000000),
};
#undef X

namespace Posix {

Serial::Serial(File file, int baud) :
	file(std::move(file))
{
	const int fd = this->file.fileno();
	const auto baud_it = baud_constants.find(baud);
	if (baud_it == baud_constants.end()) {
		throw io_error("Unsupported baud rate: " + std::to_string(baud));
	}
	/* Get current UART configuration */
	struct termios t;
	if (tcgetattr(fd, &t) < 0) {
		throw io_error("tcgetattr failed");
	}
	/* Set baud in config structure */
	if (cfsetspeed(&t, baud_it->second) < 0) {
		throw io_error("cfsetspeed failed");
	}
	/* Other configuration (no stop bit, no flow control) */
	t.c_cflag &= ~(CSTOPB | CRTSCTS);
	/* Re-configure interface */
	if (tcsetattr(fd, TCSANOW, &t) < 0) {
		throw io_error("tcsetattr failed");
	}
	/* Flush buffer */
	if (tcflush(fd, TCIOFLUSH) < 0) {
		throw io_error("tcflush failed");
	}
}

}
