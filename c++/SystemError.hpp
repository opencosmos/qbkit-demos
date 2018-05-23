#pragma once
#include <string>
#include <stdexcept>

struct SystemError :
	std::runtime_error
{
	int code;
	SystemError(const std::string& what);
};
