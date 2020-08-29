#include <CppScript/Operations.h>
#include <CppScript/TypeWrapper.h>
#include <CppScript/Context.h>
#include "VisitorMock.h"
#include "ValueAccessor.h"
#include <random>

using namespace CppScript;

/*TEST(OperationsTestCase, SumNumbers)
{
    auto sum = Element::create<Sum>();
    sum->setUp({ Element::create<TypeWrapper<long long>>(5), Element::create<TypeWrapper<long long>>(12), Element::create<TypeWrapper<long long>>(8) });
    Context context;
    auto result = sum->execute(context);
    EXPECT_EQ(getValue<long long>(result), 25);
}

TEST(OperationsTestCase, SumNumbersFromContext)
{
    auto sum = Element::create<Sum>();
    sum->setUp({ Element::create<ContextReader>("a"), Element::create<ContextReader>("b") });
    Context context;
    context["a"] = Element::create<TypeWrapper<long long>>(15);
    context["b"] = Element::create<TypeWrapper<long long>>(22);
    auto result = sum->execute(context);
    EXPECT_EQ(getValue<long long>(result), 37);
}*/

static long long calcFibonacci(int index)
{
    long long first = 1;
    long long second = 1;
    for (--index; index > 0; --index)
    {
        std::swap(first, second);
        second += first;
    }
    return second;
}

TEST(OperationsTestCase, Fibonacci)
{
    EXPECT_EQ(calcFibonacci(0), 1);
    EXPECT_EQ(calcFibonacci(1), 1);
    EXPECT_EQ(calcFibonacci(2), 2);
    EXPECT_EQ(calcFibonacci(3), 3);
    EXPECT_EQ(calcFibonacci(4), 5);
    EXPECT_EQ(calcFibonacci(5), 8);
    EXPECT_EQ(calcFibonacci(6), 13);
    EXPECT_EQ(calcFibonacci(7), 21);
    EXPECT_EQ(calcFibonacci(8), 34);
    EXPECT_EQ(calcFibonacci(9), 55);
    EXPECT_EQ(calcFibonacci(10), 89);
}

/*TEST(OperationsTestCase, FibonacciScript)
{
    auto firstValue = Element::create<TypeWrapper<long long>>(1);
    auto secondValue = Element::create<TypeWrapper<long long>>(1);

    //auto range = Element::create<RangeGenerator>();
    //range->setUp({ Element::create<TypeWrapper<long long>>(10) });
    //auto forCycle = Element::create<ForCycle>();

    auto assignFirst = Element::create<WriteElement>(0);
    auto assignSecond = Element::create<WriteElement>(1);
    auto assignSum = Element::create<WriteElement>(2);
    auto sum = Element::create<Sum>();
    auto readFirst = Element::create<ReadElement>(0);
    auto readSecond = Element::create<ReadElement>(1);
    auto readSum = Element::create<ReadElement>(2);
    assignSum->setUp({ sum });
    sum->setUp({ readFirst, readSecond });
    assignFirst->setUp({ readSecond });
    assignSecond->setUp({ readSum });
    forCycle.setUp(range, Frame{ {assignSum, assignFirst, assignSecond} });

    Runner runner{ {firstValue, secondValue, {}} };
    forCycle.accept(runner);

    //EXPECT_EQ(firstValue->get(), calcFibonacci(10));
}

TEST(ValueCastCase, CastElementToValue)
{
    TypeWrapperBase::Ref val1 = Element::create<TypeWrapper<int>>(5);
    TypeWrapperBase::Ref val2 = Element::create<TypeWrapper<int>>(10);
    EXPECT_EQ(TypeWrapper<int>::typeDescriptor.get(*val1), 5);
    EXPECT_THROW(TypeWrapper<float>::typeDescriptor.get(*val2), InvalidTypeCastOld);
}

static std::vector<int> shuffle(int count, int distance)
{
    std::default_random_engine rnd;
    std::vector<int> result(count);
    for (size_t i = 0; i < count; ++i)
    {
        int dist = (rnd() >> 3) % (2 * distance) + count - distance;
        result[i] = (i + dist) % count;
    }
    return result;
}

TEST(SortTest, VerySmallDistance)
{
    for (int i = 0; i < 10000; ++i)
    {
        auto data = shuffle(10000, 10);
        std::sort(data.begin(), data.end());
    }
}

TEST(SortTest, SmallDistance)
{
    for (int i = 0; i < 10000; ++i)
    {
        auto data = shuffle(10000, 100);
        std::sort(data.begin(), data.end());
    }
}
TEST(SortTest, LargeDistance)
{
    for (int i = 0; i < 10000; ++i)
    {
        auto data = shuffle(10000, 10000);
        std::sort(data.begin(), data.end());
    }
}*/
