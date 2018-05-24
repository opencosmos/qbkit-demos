#pragma once

/*
 * Exception type for errors where extra information is provided by errno.
 */

#include <string>
#include <stdexcept>

struct SystemError :
	std::runtime_error
{
	int code;
	SystemError(const std::string& what);
};
