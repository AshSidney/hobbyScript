#pragma once

#include <CppScript/IntValue.h>

using namespace CppScript;
    
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
