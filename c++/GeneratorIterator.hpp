#pragma once

/*
 * Adapter to provide a sequential output-iterator wrapper around a simple
 * consumer functor.
 */

#include <functional>

namespace Util {

/* Consumer */
template <typename Data>
class GeneratorIterator
{
	std::function<void(Data)> sender;
public:
	GeneratorIterator(std::function<void(Data)> sender) :
		sender(std::move(sender))
	{
	}
	GeneratorIterator& operator ++ () { return *this; }
	GeneratorIterator& operator ++ (int) { return *this; }
	GeneratorIterator& operator * () { return *this; }
	void operator = (Data data) { sender(data); }
};

}
