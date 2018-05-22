#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

#include <mutex>
#include <condition_variable>

namespace Util {

class RingBuffer
{
	std::vector<std::uint8_t> buffer;
	std::size_t start;
	std::size_t length;
	/*
	 * Only supports one reader and one writer.
	 *
	 * To support multiple of either, add an extra mutex which is locked
	 * inside of mx for reading, and another which is locked inside of mx
	 * for writing.  Both of those mutexes should remain locked until the
	 * read/write operation is complete, in order to avoid mixing of
	 * reads/writes.
	 */
	mutable std::mutex mx;
	std::condition_variable read_cv;
	std::condition_variable write_cv;
	class iterator
	{
		RingBuffer& self;
		std::size_t idx;
	public:
		iterator(RingBuffer& self, std::size_t idx) :
			self(self), idx(idx) { }
		bool operator == (const iterator& it) const
			{ return idx == it.idx; }
		bool operator != (const iterator& it) const
			{ return ! operator == (it); }
		iterator& operator ++ ()
		{
			if (++idx == self.buffer.size()) {
				idx = 0;
			}
			return *this;
		}
		iterator operator ++ (int)
		{
			iterator ret = *this;
			operator ++ ();
			return ret;
		}
		std::uint8_t& operator * () const
			{ return self.buffer[idx]; }

	};
	iterator begin()
	{
		return { *this, start };
	}
	iterator end()
	{
		return { *this, length };
	}
public:
	RingBuffer(std::size_t capacity) :
		buffer(capacity),
		start(0),
		length(0)
	{
		buffer.resize(capacity);
	}
	void clear()
	{
		std::unique_lock<std::mutex> lock(mx);
		length = 0;
	}
	template <typename InputIt>
	void write(InputIt first, InputIt last)
	{
		std::unique_lock<std::mutex> lock(mx);
		auto& it = first;
		auto out = end();
		while (it != last) {
			while (length == buffer.size()) {
				write_cv.wait(lock);
			}
			*out++ = *it++;
			++length;
		}
		read_cv.notify_all();
	}
	template <typename OutputIt>
	void read(OutputIt out)
	{
		std::unique_lock<std::mutex> lock(mx);
		auto it = begin();
		auto last = end();
		bool taken = false;
		while (it != last && !(taken && length == 0)) {
			while (length == 0) {
				read_cv.wait(lock);
			}
			*out++ = *it++;
			--length;
			taken = true;
		}
		write_cv.notify_all();
	}
	std::size_t size() const
	{
		std::lock_guard<std::mutex> lock(mx);
		return length;
	}
	std::size_t space() const
	{
		return buffer.size() - size();
	}
	bool empty() const
	{
		return size() == 0;
	}
	bool full() const
	{
		return size() == buffer.size();
	}
};

}
