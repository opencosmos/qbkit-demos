#pragma once
#include <functional>

namespace Util {

/* Consumer */
template <typename Data>
class GeneratorIterator
{
	std::function<void(Data)> sender;
public:
	GeneratorIterator(decltype(sender) sender) :
		sender(std::move(sender))
	{ 
	}
	GeneratorIterator& operator ++ () { return *this; }
	GeneratorIterator& operator ++ (int) { return *this; }
	GeneratorIterator& operator * () { return *this; }
	void operator = (Data data) { sender(data); }
};

}
