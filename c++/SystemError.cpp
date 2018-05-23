#include <cerrno>
#include <cstring>

#include "SystemError.hpp"

SystemError::SystemError(const std::string& what) :
	std::runtime_error(what + ": " + strerror(errno))
{
	code = errno;
}
