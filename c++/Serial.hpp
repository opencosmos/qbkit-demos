#pragma once
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

