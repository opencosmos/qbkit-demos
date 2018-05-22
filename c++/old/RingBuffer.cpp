#include "RingBuffer.hpp"

namespace Util {

RingBuffer::RingBuffer(std::size_t capacity) :
	buffer(std::make_unique<std::uint8_t[]>(capacity)),
	capacity(capacity),
	start(0),
	length(0)
{
}

void RingBuffer::clear()
{
	start = 0;
	length = 0;
}

void RingBuffer::write(const void *buf, std::size_t size)
{
	const auto begin = static_cast<const std::uint8_t *>(buf);
	const auto end = begin + size;
	for (auto it = begin; it != end; ++it) {
		if (length == capacity) {
			throw std::out_of_range("Ringbuffer overflow");
		}
		auto idx = start + length;
		if (idx >= capacity) {
			idx -= capacity;
		}
		buffer[idx] = *it;
		++length;
	}
}

void RingBuffer::read(void *buf, std::size_t count)
{
	const auto begin = static_cast<std::uint8_t *>(buf);
	const auto end = begin + count;
	for (auto it = begin; it != end; ++it) {
		if (length == 0) {
			throw std::out_of_range("Ringbuffer underflow");
		}
		auto idx = start + length;
		if (idx >= capacity) {
			idx -= capacity;
		}
		*it = buffer[idx];
		++start;
		--length;
	}

}

void RingBuffer::read(std::vector<uint8_t>& data)
{
	data.resize(std::min(data.capacity(), length));
	read(data.data(), data.size());
}

void RingBuffer::write(const std::vector<uint8_t>& data)
{
	write(data.data(), data.size());
}

std::size_t RingBuffer::size() const
{
	return length;
}

std::size_t RingBuffer::space() const
{
	return capacity - length;
}

bool RingBuffer::empty() const
{
	return length == 0;
}

bool RingBuffer::full() const
{
	return length == capacity;
}

}
