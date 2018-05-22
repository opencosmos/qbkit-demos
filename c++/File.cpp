#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "SystemError.hpp"

#include "File.hpp"

namespace Posix {

int File::fileno() const
{
	return fd;
}

File::File(int fd) :
	fd(fd)
{
	if (fd == -1) {
		throw SystemError("Failed to open file");
	}
	::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL) | O_NONBLOCK);
}

File::File(const std::string& path, int flags, int mode) :
	File(open(path.c_str(), flags, mode))
{
}

File::File(const File& dir, const std::string& path, int flags, int mode) :
	File(openat(dir.fd, path.c_str(), flags, mode))
{
}

File::~File()
{
	close();
}

File::File(File&& f) :
	fd(-1)
{
	std::swap(fd, f.fd);
}

File& File::operator = (File&& f)
{
	close();
	std::swap(fd, f.fd);
	return *this;
}

File::File(File &f) :
	File(dup(f.fd))
{
}

File& File::operator = (File& f) {
	close();
	f.fd = dup(f.fd);
	return *this;
}

void File::close()
{
	if (fd == -1) {
		return;
	}
	int f = fd;
	fd = -1;
	if (::close(f) == -1) {
		throw SystemError("Failed to close file");
	}
}

bool File::read(std::vector<std::uint8_t>& buf)
{
	buf.resize(buf.capacity());
	auto result = ::read(fd, buf.data(), buf.size());
	if (result == -1) {
		buf.clear();
		throw SystemError("Failed to read file");
	}
	buf.resize(result);
	return result > 0;
}

std::size_t File::write(const std::vector<std::uint8_t>& buf)
{
	auto result = ::write(fd, buf.data(), buf.size());
	if (result == -1) {
		throw SystemError("Failed to write file");
	}
	return result;
}

std::size_t File::seek(std::size_t offset, Whence whence)
{
	const auto result = ::lseek(fd, offset, whence);
	if (result == (off_t) -1) {
		throw SystemError("Failed to seek file");
	}
	return result;
}

std::size_t File::tell() const
{
	const auto result = ::lseek(fd, 0, SEEK_CUR);
	if (result == (off_t) -1) {
		throw SystemError("Failed to get file position");
	}
	return result;
}


Stat File::stat() const
{
	struct stat info;
	if (fstat(fd, &info) == -1) {
		throw SystemError("Failed to stat file");
	}
	return info;
}

}

int std::hash<Posix::File>::operator () (const Posix::File& file) const
{
	return std::hash<int>{}(file.fileno());
}
