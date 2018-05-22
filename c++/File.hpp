#pragma once
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <cerrno>
#include <cstring>

#include <string>
#include <stdexcept>
#include <vector>

namespace Posix {

struct io_error :
	std::runtime_error
{
	int code;
	io_error(const std::string& what) :
		std::runtime_error(what + ": " + strerror(errno))
	{
		code = errno;
	}
};

using Stat = struct stat;

class File
{
	int fd;
public:
	int fileno() const
	{
		return fd;
	}
	enum Whence {
		absolute = SEEK_SET,
		relative = SEEK_CUR,
		from_end = SEEK_END
	};
	File(int fd) :
		fd(fd)
	{
		if (fd == -1) {
			throw io_error("Failed to open file");
		}
		::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL) | O_NONBLOCK);
	}
	File(const std::string& path, int flags, int mode = 0) :
		File(open(path.c_str(), flags, mode))
	{
	}
	File(const File& dir, const std::string& path, int flags, int mode = 0) :
		File(openat(dir.fd, path.c_str(), flags, mode))
	{
	}
	~File()
	{
		close();
	}
	File(File&& f) :
		fd(-1)
	{
		std::swap(fd, f.fd);
	}
	File& operator = (File&& f)
	{
		close();
		std::swap(fd, f.fd);
		return *this;
	}
	File(File &f) :
		File(dup(f.fd))
	{
	}
	File& operator = (File& f) {
		close();
		f.fd = dup(f.fd);
		return *this;
	}
	void close()
	{
		if (fd == -1) {
			return;
		}
		int f = fd;
		fd = -1;
		if (::close(f) == -1) {
			throw io_error("Failed to close file");
		}
	}
	bool read(std::vector<std::uint8_t>& buf)
	{
		buf.resize(buf.capacity());
		auto result = ::read(fd, buf.data(), buf.size());
		if (result == -1) {
			buf.clear();
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				result = 0;
			} else {
				throw io_error("Failed to read file");
			}
		}
		buf.resize(result);
		return result > 0;
	}
	std::size_t write(const std::vector<std::uint8_t>& buf)
	{
		auto result = ::write(fd, buf.data(), buf.size());
		if (result == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				result = 0;
			} else {
				throw io_error("Failed to write file");
			}
		}
		return result;
	}
	std::size_t seek(std::size_t offset, Whence whence = absolute)
	{
		const auto result = ::lseek(fd, offset, whence);
		if (result == (off_t) -1) {
			throw io_error("Failed to seek file");
		}
		return result;
	}
	std::size_t tell() const
	{
		const auto result = ::lseek(fd, 0, SEEK_CUR);
		if (result == (off_t) -1) {
			throw io_error("Failed to get file position");
		}
		return result;
	}
	Stat stat() const
	{
		struct stat info;
		if (fstat(fd, &info) == -1) {
			throw io_error("Failed to stat file");
		}
		return info;
	}
};

}
