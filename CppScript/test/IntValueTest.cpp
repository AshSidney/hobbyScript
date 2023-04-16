#include <gtest/gtest.h>

#include <CppScript/IntValue.h>

using namespace CppScript;


template<typename T> std::string toString(const T& value, std::vector<std::function<std::ios_base&(std::ios_base&)>> modifiers = {})
{
	std::ostringstream ostr;
	for (const auto modifier : modifiers)
		modifier(ostr);
	ostr << value;
	return ostr.str();
}

TEST(IntValueTest, CreateFromLong)
{
	EXPECT_EQ(toString(IntValue{ 125 }), "125");
	EXPECT_EQ(toString(IntValue{ -456789 }), "-456789");

	const IntValue maxValue{ std::numeric_limits<long long>::max() };
	EXPECT_EQ(toString(maxValue), toString(std::numeric_limits<long long>::max()));
	const IntValue minValue{ std::numeric_limits<long long>::min() };
	EXPECT_EQ(toString(minValue), toString(std::numeric_limits<long long>::min()));
}

TEST(IntValueTest, CreateFromString)
{
	EXPECT_EQ(toString(IntValue{ "125" }), "125");
	EXPECT_EQ(toString(IntValue{ "-456_789" }), "-456789");

	const IntValue bigValue{ "0x123456789abcdef01234" };
	EXPECT_FALSE(bigValue.getStdInt().has_value());
	EXPECT_EQ(toString(bigValue, { std::hex, std::showbase }), "0x123456789abcdef01234");
	EXPECT_EQ(toString(bigValue, { std::hex }), "123456789abcdef01234");
	EXPECT_EQ(toString(bigValue, {}), "85968058283706962416180");

	const IntValue bigNegative{ "-987000654000321000123456789" };
	EXPECT_FALSE(bigNegative.getStdInt().has_value());
	EXPECT_EQ(toString(bigNegative, { std::showbase }), "-987000654000321000123456789");
	EXPECT_EQ(toString(bigNegative, { std::hex, std::showbase }), "-0x3306d84aa63fb1406ccdd15");
	EXPECT_EQ(toString(bigNegative, { std::hex }), "-3306d84aa63fb1406ccdd15");
}

TEST(IntValueTest, CreateLiteral)
{
	EXPECT_EQ(*765'342_I.getStdInt(), 76'53'42);

	const auto bigValue = -0x123456789abcdef01234_I;
	EXPECT_FALSE(bigValue.getStdInt());
	EXPECT_EQ(toString(bigValue, { std::hex }), "-123456789abcdef01234");
}

TEST(IntValueTest, Add)
{
	EXPECT_EQ(369258_I + 741852963_I, 742222221_I);
	EXPECT_EQ(7'538'691'4275'386_I + 789'012'369'874'569'874'123_I, 789'012'445'261'484'149'509_I);
	EXPECT_EQ(IntValueX{ "75386914275386000000" } + IntValueX{ 123456 }, IntValueX{ "75386914275386123456" });
	EXPECT_EQ(IntValueX{ "9516237849516237840951623" } + IntValueX{ "-0xabcdef0123456789ab" }, IntValueX{ "9513068617199085298655132" });
}

TEST(IntValueTest, Subtract)
{
	EXPECT_EQ(IntValue{ 666555444 } - IntValue{ 7778889990 }, IntValue{ -7112334546 });
	EXPECT_EQ(IntValue{ -0x777888999 } - IntValue{ "-0x11114444777722225555" }, IntValue{ "0x11114444776faa99cbbc" });
	EXPECT_EQ(75'386'914'275'386'000'000_I - 123'456_I, 75'386'914'275'385'876'544_I);
	EXPECT_EQ(IntValue{ "0xababababababababababababababababababababababab" } - IntValue{ "0x1111222233334444ffffeeeeddddcccc" },
		IntValue{ "0xababababababab9a9a898978786766ababbcbccdcddedf" });
}

TEST(IntValueTest, Multiply)
{
	EXPECT_EQ(789456123741852963_I * 444555666777333222111_I, 350957193481508328424053247714382464893_I);
	EXPECT_EQ(1254786223547821569875332658632365232468789524_I * -7551332655336652545662566545522002012015320053335500264566332236_I,
		-9475308185343221951132656218327652005248281845700277325013826435641211378264809264598317133135143707340295664_I);
	EXPECT_EQ(-7563224578200145622874002545_I * -32105578954230586220015_I, 242821703843982097876090511453795501897465039938175_I);
}

TEST(IntValueTest, DivideAndModulo)
{
	IntValue divValue { 12345678975 };
	EXPECT_EQ(divValue.divide(456789_I), IntValue(12345678975 / 456789));
	EXPECT_EQ(divValue, IntValue(12345678975 % 456789));

	EXPECT_EQ(-42635179854_I / 65859_I, IntValue(-42635179854 / 65859));
	EXPECT_EQ(7521463_I / -951436_I, IntValue(7521463 / -951436));

	EXPECT_EQ(85023647_I % 106745_I, IntValue(85023647 % 106745));
	EXPECT_EQ(-753241_I % -30124_I, IntValue(-753241 % -30124));

	EXPECT_EQ(756_I / -62014_I, 0_I);
}

TEST(IntValueTest, DivideAndModulo_Big)
{
	IntValue divValue { 123456789750147025836901478524620147892133024862365520076325_I };
	EXPECT_EQ(divValue.divide(7536942001567620054453220075342_I), 16380222870823347183537501302_I);
	EXPECT_EQ(divValue, 6570549245971224712190656981041_I);

	IntValue divLargeRem{ 0x10fffffffef0123456789abcdef0_I };
	EXPECT_EQ(divLargeRem.divide(0xffffffffff_I), 0x10ffffffff01123456_I);
	EXPECT_EQ(divLargeRem, 0x779bcf1346_I);

	IntValue divSmallRem{ 0xffffffffffffffffffff_I };
	EXPECT_EQ(divSmallRem.divide(0xffffffff000000000000_I), 1_I);
	EXPECT_EQ(divSmallRem, 0xffffffffffff_I);
}

TEST(IntValueTest, DivideAndModulo_MultipleTimes)
{
	IntValue baseValue { 999888777666555444333222111000123456789987654321000009876543210_I};
	for (IntValue div { 1000000000000000000000_I }; div < 1000000000000000100000_I; div += IntValue{1})
	{
		IntValue divided = baseValue;
		auto result = divided.divide(div);
		EXPECT_EQ(divided + div * result, baseValue);
	}
}


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

template <typename T> T fibonacciPtr(size_t count)
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

IntValue::SegType fibonacciSegType(size_t count)
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

const char fib50[] = "12586269025";
const char fib100[] = "354224848179261915075";
const char fib200[] = "280571172992510140037611932413038677189525";
const char fib1000[] = "434665576869374564356885276750406258025646605173717804024817290895365554179490518904038798400\
79255169295922593080322634775209689623239873322471161642996440906533187938298969649928516003704476137795166849228875";

TEST(IntValueTest, Fibonacci)
{
	EXPECT_EQ(fibonacci<IntValue>(5), 5_I);
	EXPECT_EQ(fibonacci<IntValue>(10), 55_I);
	EXPECT_EQ(fibonacci<IntValue>(50), IntValue{fib50});
	EXPECT_EQ(fibonacci<IntValue>(100), IntValue{fib100});
	EXPECT_EQ(fibonacci<IntValue>(200), IntValue{fib200});
}

TEST(IntValuePerformanceTest, Fibonacci_Small_SegType)
{
	const IntValue::SegType result{ 12586269025 };
	for (int i = 0; i < 100000; ++i)
		EXPECT_EQ(fibonacciSegType(50), result);
}

TEST(IntValuePerformanceTest, Fibonacci_Small)
{
	const IntValue result{ fib50 };
	for (int i = 0; i < 100000; ++i)
		EXPECT_EQ(fibonacci<IntValue>(50), result);
}

TEST(IntValuePerformanceTest, Fibonacci2_Small)
{
	const IntValue result{ fib50 };
	for (int i = 0; i < 100000; ++i)
		EXPECT_EQ(fibonacci2<IntValue>(50), result);
}

TEST(IntValuePerformanceTest, Fibonacci_Middle)
{
	const IntValue result { fib200 };
	for (int i = 0; i < 100000; ++i)
		EXPECT_EQ(fibonacci<IntValue>(200), result);
}

TEST(IntValuePerformanceTest, Fibonacci_Big)
{
	const IntValue result { fib1000 };
	for (int i = 0; i < 100000; ++i)
		EXPECT_EQ(fibonacci<IntValue>(1000), result);
}

TEST(IntValuePerformanceTest, Fibonacci_SmallPtr)
{
	const IntValue result { fib50 };
	for (int i = 0; i < 100000; ++i)
		EXPECT_EQ(fibonacciPtr<IntValue>(50), result);
}

TEST(IntValuePerformanceTest, Fibonacci_MiddlePtr)
{
	const IntValue result { fib200 };
	for (int i = 0; i < 100000; ++i)
		EXPECT_EQ(fibonacciPtr<IntValue>(200), result);
}

TEST(IntValuePerformanceTest, Fibonacci_BigPtr)
{
	const IntValue result { fib1000 };
	for (int i = 0; i < 100000; ++i)
		EXPECT_EQ(fibonacciPtr<IntValue>(1000), result);
}

using topBitFnc = unsigned int(IntValue::SegType);
unsigned int evalTopBit(topBitFnc fnc)
{
	unsigned int rslt = 0;
	for (unsigned int bit = 16; bit < 64; ++bit)
		for (IntValue::SegType val = 0; val < 50000; ++val)
			rslt = fnc((1ull << bit) + val);
	return rslt;
}




IntValueX::LongValue fibonacci2(size_t count)
{
	IntValueX::LongValue first{ 0 };
	if (count == 0)
		return first;
	IntValueX::LongValue second{ 1 };
	for (--count; count > 0; --count)
	{
		first.addSubtract(second, true);
		std::swap(first, second);
	}
	return second;
}

IntValueX::StandardValue fibonacci3(size_t count)
{
	IntValueX::StandardValue first{ 0 };
	if (count == 0)
		return first;
	IntValueX::StandardValue second{ 1 };
	for (--count; count > 0; --count)
	{
		first.value += second.value;
		std::swap(first, second);
	}
	return second;
}

IntValueX::StandardValue fibonacci4(size_t count)
{
	IntValueX::StandardValue first{ 0 };
	if (count == 0)
		return first;
	IntValueX::StandardValue second{ 1 };
	for (--count; count > 0; --count)
	{
		first.value += second.value;
		std::swap(first.value, second.value);
	}
	return second;
}

TEST(IntValueXTest, Fibonacci_Performance0Small)
{
	const long long result{ 12586269025 };
	for (int i = 0; i < 1000000; ++i)
		EXPECT_EQ(fibonacci<long long>(50), result);
}

TEST(IntValueXTest, Fibonacci_Performance1Small)
{
	const IntValueX result{ fib50 };
	for (int i = 0; i < 100000; ++i)
		EXPECT_EQ(fibonacci<IntValueX>(50), result);
}

TEST(IntValueXTest, Fibonacci_Performance2Small)
{
	const IntValueX::LongValue result{ {12586269025}, true };
	for (int i = 0; i < 100000; ++i)
		EXPECT_TRUE(fibonacci2(50).equal(result));
}

TEST(IntValueXTest, Fibonacci_Performance3Small)
{
	const IntValueX::StandardValue result{ 12586269025 };
	for (int i = 0; i < 100000; ++i)
		EXPECT_EQ(fibonacci3(50).value, result.value);
}

TEST(IntValueXTest, Fibonacci_Performance4Small)
{
	const IntValueX::StandardValue result{ 12586269025 };
	for (int i = 0; i < 100000; ++i)
		EXPECT_EQ(fibonacci4(50).value, result.value);
}

TEST(IntValueXTest, Fibonacci_Performance1Mid)
{
	const IntValueX result{ fib200 };
	for (int i = 0; i < 100000; ++i)
		EXPECT_EQ(fibonacci<IntValueX>(200), result);
}

TEST(IntValueXTest, Fibonacci_Performance1Big)
{
	const IntValueX result{ fib1000 };
	for (int i = 0; i < 100000; ++i)
		EXPECT_EQ(fibonacci<IntValueX>(1000), result);
}

TEST(IntValueXTest, Fibonacci_Performance2Mid)
{
	const IntValueX::LongValue result{ {0xf067cb83df17e395, 0x864a5c1caeb07d0e, 0x338}, true };
	for (int i = 0; i < 100000; ++i)
		EXPECT_TRUE(fibonacci2(200).equal(result));
}
