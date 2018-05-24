#pragma once

/*
 * Wrapper around UNIX file descriptor, with error-handling.
 *
 * Configures the descriptor for non-blocking IO, so the read/write calls should
 * be non-blocking as a result.
 */

#include <string>
#include <vector>

namespace Posix {

using Stat = struct stat;

/*
 * Owns a file descriptor.
 *
 * No two File instances should have the same
 * descriptor.
 *
 * Copy-constructor uses dup().
 */
class File
{
	int fd;
public:
	int fileno() const;
	enum Whence {
		absolute = SEEK_SET,
		relative = SEEK_CUR,
		from_end = SEEK_END
	};
	File(int fd);
	File(const std::string& path, int flags, int mode = 0);
	File(const File& dir, const std::string& path, int flags, int mode = 0);
	~File();
	File(File&& f);
	File& operator = (File&& f);
	File(File &f);
	File& operator = (File& f);
	void close();
	bool read(std::vector<std::uint8_t>& buf);
	std::size_t write(const std::vector<std::uint8_t>& buf);
	std::size_t seek(std::size_t offset, Whence whence = absolute);
	std::size_t tell() const;
	Stat stat() const;
};

}

template <>
struct std::hash<Posix::File> {
	int operator () (const Posix::File& file) const;
};
