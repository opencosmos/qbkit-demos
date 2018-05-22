#pragma once
#include <vector>
#include <cstddef>
#include <cstdint>
#include <functional>

#include "GeneratorIterator.hpp"

namespace Util {

/*
 * Provides an output iterator.
 *
 * Corks data up to a pre-set capacity, then
 * flushes all the corked data at once, clearing the buffer afterwards to that
 * it is ready to start corking more data.
 */
class Cork
{
	std::size_t capacity;
	std::vector<std::uint8_t> data;
	std::function<void()> on_flush;
	using iterator_type = GeneratorIterator<std::uint8_t>;
	iterator_type flush_iterator;
public:
	template <typename Flush>
	Cork(std::size_t capacity, Flush flush) :
		capacity(capacity),
		data(capacity),
		on_flush([this, flush] () { flush(data.cbegin(), data.cend()); }),
		flush_iterator([this] (const auto& data) {
			push_back(data);
		})
	{
	}
	void push_back(std::uint8_t byte)
	{
		data.push_back(byte);
		if (data.size() == capacity) {
			flush();
		}
	}
	void flush()
	{
		on_flush();
		data.clear();
	}
	iterator_type inserter()
	{
		return flush_iterator;
	}
};

}
