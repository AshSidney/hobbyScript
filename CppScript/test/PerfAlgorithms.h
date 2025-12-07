#pragma once

#include <CppScript/IntValue.h>

using namespace CppScript;

namespace CppScriptTest
{
    
template <typename T> T fibonacci(size_t count)
{
	T first{0};
	if (count == 0)
		return first;
	T second{1};
	for (--count; count > 0; --count)
	{
		first += second;
		std::swap(first, second);
	}
	return second;
}

inline IntValue fibonacciPtr(size_t count)
{
	IntValue frst {0};
	if (count == 0)
		return frst;
	IntValue scnd {1};
	IntValue* first = &frst;
	IntValue* second = &scnd;
	for (--count; count > 0; --count)
	{
		*first += *second;
		std::swap(first, second);
	}
	return *second;
}

inline IntValue::SegType fibonacciSegType(size_t count)
{
	IntValue::SegType first{0};
	if (count == 0)
		return first;
	IntValue::SegType second{1};
	for (--count; count > 0; --count)
	{
		IntValue::SegType temp;
		std::tie(first, temp) = IntValue::add(first, second);
		std::swap(first, second);
	}
	return second;
}

template <typename T> T fibonacci2(int count)
{
	T first{0};
	T second{1};
	for (; count > 1; count -= 2)
	{
		first += second;
		second += first;
	}
	return count == 0 ? first : second;
}

inline IntValue fibonacci2Ptr(size_t count)
{
	IntValue frst {0};
	IntValue scnd {1};
	IntValue* first = &frst;
	IntValue* second = &scnd;
	for (; count > 1; count -= 2)
	{
		*first += *second;
		*second += *first;
	}
	return count == 0 ? *first : *second;
}

const char fib50[] = "12586269025";
const char fib89[] = "1779979416004714189";
const char fib100[] = "354224848179261915075";
const char fib200[] = "280571172992510140037611932413038677189525";
const char fib1000[] = "434665576869374564356885276750406258025646605173717804024817290895365554179490518904038798400\
79255169295922593080322634775209689623239873322471161642996440906533187938298969649928516003704476137795166849228875";


template <typename T> T factorial(long n)
{
	T result{ n };
	for (--n; n > 1; --n)
		result *= n;
	return result;
}

const char fact20[]{ "2432902008176640000" };
const char fact50[]{ "30414093201713378043612608166064768844377641568960512000000000000" };
const char fact100[]{ "933262154439441526816992388562667004907159682643816214685929638952175999932299156089414639761565\
18286253697920827223758251185210916864000000000000000000000000" };
const char fact200[]{ "78865786736479050355236321393218506229513597768717326329474253324435944996340334292030428401198462390417721213891\
9638830257642790242637105061926624952829931113462857270763317237396988943922445621451664240254033291864131227428294853277524242407573903\
240321257405579568660226031904170324062351700858796178922222789623703897374720000000000000000000000000000000000000000000000000" };

}