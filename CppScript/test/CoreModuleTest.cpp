#include <gtest/gtest.h>

#include <CppScript/CoreModule.h>
#include <CppScript/ValueHolder.h>
#include <CppScript/IntValue.h>
#include "Fibonacci.h"

using namespace CppScript;


class CoreModuleFixture : public testing::Test
{
protected:
    Module testModule{createCoreModule()};

    std::tuple<CodeBlock, PlaceData, PlaceData> createFibonacci(ExecutionContext& context, const IntValue& count,
        EnumFlag<FunctionOptions> options = {})
    {
        CodeBlock code;
        const auto placeStart0 = code.addPlaceValue(0_I);
        const auto placeStart1 = code.addPlaceValue(1_I);
        const auto placeStart2 = code.addPlaceValue(2_I);
        const auto placeCountRef = code.addPlaceValue(count);
        const auto placeCount = code.addPlaceType(SpecTypeValueHolder<IntValue>::specTypeId);
        const auto placeFirst = code.addPlaceType(SpecTypeValueHolder<IntValue>::specTypeId);
        const auto placeSecond = code.addPlaceType(SpecTypeValueHolder<IntValue>::specTypeId);
        FunctionContext funcCont{"copy", options, placeFirst, {placeStart0}, {}, &code};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont.returnPlace = placeSecond;
        funcCont.argPlaces = {placeStart1};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont.returnPlace = placeCount;
        funcCont.argPlaces = {placeCountRef};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"<=>", options | EnumFlag{FunctionOptions::Jump}, {PlaceType::Void}, {placeCount, placeStart1}, {5, 6, 1}, &code};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"+=", options, {PlaceType::Void}, {placeFirst, placeSecond}, {}, &code};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"+=", options, {PlaceType::Void}, {placeSecond, placeFirst}, {}, &code};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"-=", options, {PlaceType::Void}, {placeCount, placeStart2}, {}, &code};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"<=>", options | EnumFlag{FunctionOptions::Jump}, {PlaceType::Void}, {placeCount, placeStart1}, {1, 2, -3}, &code};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"=", options, {PlaceType::Void}, {placeSecond, placeFirst}, {}, &code};
        code.operations.push_back(testModule.buildFunction(funcCont));
        return { std::move(code), placeCountRef, placeSecond };
    }
};

TEST_F(CoreModuleFixture, IntCreateAndAdd)
{
    CodeBlock code;
    const auto placeLocal = code.addPlaceType(SpecTypeValueHolder<IntValue>::specTypeId);
    const auto placeLocal2 = code.addPlaceType(SpecTypeValueHolder<IntValue>::specTypeId);
    const auto placeLit = code.addPlaceValue(std::string{"111222333444"});
    FunctionContext funcCont{"int", {}, placeLocal, {placeLit}, {}, &code};
    code.operations.push_back(testModule.buildFunction(funcCont));
    const auto placeLit2 = code.addPlaceValue(98765_I);
    funcCont.name = "copy";
    funcCont.returnPlace = placeLocal2;
    funcCont.argPlaces = {placeLit2};
    code.operations.push_back(testModule.buildFunction(funcCont));
    funcCont.name = "+=";
    funcCont.returnPlace = {PlaceType::Void, 0};
    funcCont.argPlaces = {placeLocal2, placeLocal};
    code.operations.push_back(testModule.buildFunction(funcCont));

    ExecutionContext execCont;
    execCont.run(code);

    EXPECT_EQ(static_cast<TypeValueHolder<IntValue>&>(execCont.get(placeLocal)).get(), 111222333444_I);
    EXPECT_EQ(static_cast<TypeValueHolder<IntValue>&>(execCont.get(placeLocal2)).get(), 111222432209_I);
}

TEST_F(CoreModuleFixture, FloatCreateAndAdd)
{
    CodeBlock code;
    const auto placeLocal = code.addPlaceType(SpecTypeValueHolder<FloatValue>::specTypeId);
    const auto placeFloat = code.addPlaceType(SpecTypeValueHolder<FloatValue>::specTypeId);
    const auto placeLit = code.addPlaceValue(std::string{"12.34e5"});
    const auto placeLit2 = code.addPlaceValue(FloatValue{-78901.23});
    FunctionContext funcCont{"float", {}, placeLocal, {placeLit}, {}, &code};
    code.operations.push_back(testModule.buildFunction(funcCont));
    funcCont.name = "copy";
    funcCont.returnPlace = placeFloat;
    funcCont.argPlaces = {placeLit2};
    code.operations.push_back(testModule.buildFunction(funcCont));
    funcCont.name = "+=";
    funcCont.returnPlace = {PlaceType::Void, 0};
    funcCont.argPlaces = {placeFloat, placeLocal};
    code.operations.push_back(testModule.buildFunction(funcCont));

    ExecutionContext execCont;
    execCont.run(code);

    EXPECT_EQ(static_cast<TypeValueHolder<FloatValue>&>(execCont.get(placeLocal)).get(), 12.34e5);
    EXPECT_EQ(static_cast<TypeValueHolder<FloatValue>&>(execCont.get(placeFloat)).get(), 12.34e5 - 78901.23);
}

TEST_F(CoreModuleFixture, Fibonacci)
{
    ExecutionContext context;
    const IntValue count1{50_I};
    auto [code, countPlace, resultPlace] = createFibonacci(context, count1);
    context.run(code);
    auto& result = static_cast<TypeValueHolder<IntValue>&>(context.get(resultPlace));
    EXPECT_EQ(result.get(), 12586269025_I);

    auto& count = static_cast<SpecTypeValueHolder<const IntValue&>&>(context.get(countPlace));
    const IntValue count2 = 89_I;
    count.set(count2);
    context.run();
    EXPECT_EQ(result.get(), 1779979416004714189_I);

    const IntValue count3 = 200_I;
    count.set(count3);
    context.run();
    EXPECT_EQ(result.get(), 280571172992510140037611932413038677189525_I);
}

TEST_F(CoreModuleFixture, Fibonacci_Cache)
{
    ExecutionContext context;
    const IntValue count1{50_I};
    auto [code, countPlace, resultPlace] = createFibonacci(context, count1, {FunctionOptions::Cache});
    context.run(code);
    auto& result = static_cast<TypeValueHolder<IntValue>&>(context.get(resultPlace));
    EXPECT_EQ(result.get(), 12586269025_I);

    auto& count = static_cast<SpecTypeValueHolder<const IntValue&>&>(context.get(countPlace));
    const IntValue count2 = 89_I;
    count.set(count2);
    context.run();
    EXPECT_EQ(result.get(), 1779979416004714189_I);

    const IntValue count3 = 200_I;
    count.set(count3);
    context.run();
    EXPECT_EQ(result.get(), 280571172992510140037611932413038677189525_I);
}


struct FibonacciParams
{
    EnumFlag<FunctionOptions> options;
    IntValue count;
    IntValue result;
};

class CoreModulePerformanceFixture : public CoreModuleFixture, public testing::WithParamInterface<FibonacciParams>
{
protected:
    const int repeats{100000};
};

TEST_P(CoreModulePerformanceFixture, Fibonacci)
{
    ExecutionContext context;
    auto [code, countPlace, resultPlace] = createFibonacci(context, GetParam().count, GetParam().options);
    context.run(code);
    const auto& resultRef = static_cast<TypeValueHolder<IntValue>&>(context.get(resultPlace));
    EXPECT_EQ(resultRef.get(), GetParam().result);

	for (int i = 0; i < repeats; ++i)
    {
        context.run();
    }
    EXPECT_EQ(resultRef.get(), GetParam().result);
}

INSTANTIATE_TEST_SUITE_P(FibonacciInstances, CoreModulePerformanceFixture,
    testing::Values(FibonacciParams{{}, 50_I, 12586269025_I},
        FibonacciParams{{FunctionOptions::Cache}, 50_I, 12586269025_I},
        FibonacciParams{{}, 200_I, fibonacci2<IntValue>(200)},
        FibonacciParams{{FunctionOptions::Cache}, 200_I, fibonacci2<IntValue>(200)},
        FibonacciParams{{}, 1000_I, fibonacci2<IntValue>(1000)},
        FibonacciParams{{FunctionOptions::Cache}, 1000_I, fibonacci2<IntValue>(1000)}));



long long assign(const long long& val)
{
    return val;
}

void add(long long& left, const long long& right)
{
    left += right;
}

void swap(long long& left, long long& right)
{
    std::swap(left, right);
}

bool greater(const long long& left, const long long& right)
{
    return left > right;
}

TEST_F(CoreModulePerformanceFixture, Fibonacci_OldComparison)
{
    Module legacyMod{"legacy"};
    legacyMod.defFunction("=", &assign)
        .defFunction("+=", &add)
        .defFunction("swap", &swap)
        .defFunction(">", &greater);

    CodeBlock code;
    const auto placeStart0 = code.addPlaceValue(long long(0));
    const auto placeStart1 = code.addPlaceValue(long long(1));
    const auto placeStart2 = code.addPlaceValue(long long(-1));
    const auto placeCountConst = code.addPlaceValue(long long(90));
    const auto placeCount = code.addPlaceType(SpecTypeValueHolder<long long>::specTypeId);
    const auto placeFirst = code.addPlaceType(SpecTypeValueHolder<long long>::specTypeId);
    const auto placeSecond = code.addPlaceType(SpecTypeValueHolder<long long>::specTypeId);
    FunctionContext funcCont{"=", {FunctionOptions::Cache}, placeFirst, {placeStart0}, {}, &code};
    code.operations.push_back(legacyMod.buildFunction(funcCont));
    funcCont.returnPlace = placeSecond;
    funcCont.argPlaces = {placeStart1};
    code.operations.push_back(legacyMod.buildFunction(funcCont));
    funcCont.returnPlace = placeCount;
    funcCont.argPlaces = {placeCountConst};
    code.operations.push_back(legacyMod.buildFunction(funcCont));
    funcCont = {"+=", {FunctionOptions::Cache}, {PlaceType::Void}, {placeFirst, placeSecond}, {}, &code};
    code.operations.push_back(legacyMod.buildFunction(funcCont));
    funcCont = {"swap", {FunctionOptions::Cache}, {PlaceType::Void}, {placeFirst, placeSecond}, {}, &code};
    code.operations.push_back(legacyMod.buildFunction(funcCont));
    funcCont = {"+=", {FunctionOptions::Cache}, {PlaceType::Void}, {placeCount, placeStart2}, {}, &code};
    code.operations.push_back(legacyMod.buildFunction(funcCont));
    funcCont = {">", {FunctionOptions::Cache, FunctionOptions::Jump}, {PlaceType::Void}, {placeCount, placeStart1}, {-3, 1}, &code};
    code.operations.push_back(legacyMod.buildFunction(funcCont));

    ExecutionContext context;
    context.run(code);
    const auto& resultRef = static_cast<TypeValueHolder<long long>&>(context.get(placeSecond));
    EXPECT_EQ(resultRef.get(), fibonacci2<long long>(90));

	for (size_t i = 0; i < 10000; ++i)
	{
        context.run();
	}

    EXPECT_EQ(resultRef.get(), fibonacci2<long long>(90));
}
