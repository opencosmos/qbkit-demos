#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace Util {

class RingBuffer
{
	std::unique_ptr<std::uint8_t[]> buffer;
	std::size_t capacity;
	std::size_t start;
	std::size_t length;
public:
	RingBuffer(std::size_t capacity);

	void clear();

	void write(const void *buf, std::size_t size);
	void read(void *buf, std::size_t count);

	void read(std::vector<uint8_t>& data);
	void write(const std::vector<uint8_t>& data);

	std::size_t size() const;
	std::size_t space() const;

	bool empty() const;
	bool full() const;
};

}
