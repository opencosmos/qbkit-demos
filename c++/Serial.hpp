#pragma once

/*
 * Wrapper around serial port.
 *
 * Takes ownership of an open file passed to the constructor, and configures the
 * serial interface represented by it as so:
 *  - baud: as requested
 *  - cts/rtx: no
 *  - xon/xoff: no
 *  - data bits: 8
 *  - parity bits: 0
 *  - start bits: 1
 *  - stop bits: 0
 */

#include <vector>
#include <cstdint>
#include <cstddef>
#include <mutex>

#include "File.hpp"

namespace Posix {

class Serial
{
	File file;
public:
	int fileno() const { return file.fileno(); }
	Serial(File file, int baud);
	void read(std::vector<std::uint8_t>& buffer)
	{
		file.read(buffer);
	}
	std::size_t write(const std::vector<std::uint8_t>& buffer)
	{
		return file.write(buffer);
	}
};

}

