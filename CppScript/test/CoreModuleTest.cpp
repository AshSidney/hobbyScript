#include <gtest/gtest.h>

#include <CppScript/CoreModule.h>
#include <CppScript/ValueHolder.h>
#include <CppScript/IntValue.h>
#include "Fibonacci.h"
#include "TestUtils.h"

using namespace CppScript;


class CoreModuleFixture : public testing::Test
{
protected:
    Module testModule{createCoreModule()};

    std::tuple<CodeBlock, IntValue*, IntValue*> createFibonacci(ExecutionContext& context, const IntValue& count,
        EnumFlag<FunctionOptions> options = {})
    {
        DataBlockDef::Builder builder;
        const PlaceData placeStart0{ PlaceType::Module, builder.addPlace(makeValue(0_I)) };
        const PlaceData placeStart1{ PlaceType::Module, builder.addPlace(makeValue(1_I)) };
        const PlaceData placeStart2{ PlaceType::Module, builder.addPlace(makeValue(2_I)) };
        const PlaceData placeCountRef{ PlaceType::Module, builder.addPlace(makeValue(count)) };
        const PlaceData placeCount{ PlaceType::Module, builder.addPlace(SpecTypeValueHolder<IntValue>::specTypeId) };
        const PlaceData placeFirst{ PlaceType::Module, builder.addPlace(SpecTypeValueHolder<IntValue>::specTypeId) };
        const PlaceData placeSecond{ PlaceType::Module, builder.addPlace(makeValue(0_I)) };
        auto data = builder.build();
        auto* countVal = &static_cast<SpecTypeValueHolder<IntValue>&>(*data.sourceValues[3]).get();
        auto* resultVal = &static_cast<SpecTypeValueHolder<IntValue>&>(*data.sourceValues[4]).get();
        CodeBlock code{ { std::move(data) } };
        FunctionContext funcCont{"copy", options, placeFirst, {placeStart0}, {}, &code, &code.getDataLayout()};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont.returnPlace = placeCount;
        funcCont.argPlaces = {placeCountRef};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"=", options, {PlaceType::Void}, {placeSecond, placeStart1}, {}, &code, &code.getDataLayout()};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"<=>", options | EnumFlag{FunctionOptions::Jump}, {PlaceType::Void}, {placeCount, placeStart1}, {5, 6, 1}, &code, &code.getDataLayout()};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"+=", options, {PlaceType::Void}, {placeFirst, placeSecond}, {}, &code, &code.getDataLayout()};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"+=", options, {PlaceType::Void}, {placeSecond, placeFirst}, {}, &code, &code.getDataLayout()};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"-=", options, {PlaceType::Void}, {placeCount, placeStart2}, {}, &code, &code.getDataLayout()};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"<=>", options | EnumFlag{FunctionOptions::Jump}, {PlaceType::Void}, {placeCount, placeStart1}, {1, 2, -3}, &code, &code.getDataLayout()};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"=", options, {PlaceType::Void}, {placeSecond, placeFirst}, {}, &code, &code.getDataLayout()};
        code.operations.push_back(testModule.buildFunction(funcCont));
        return { std::move(code), countVal, resultVal };
    }
};

TEST_F(CoreModuleFixture, IntCreateAndAdd)
{
    DataBlockDef::Builder builder;
    const PlaceData place1{ PlaceType::Module, builder.addPlace(SpecTypeValueHolder<IntValue>::specTypeId) };
    const PlaceData place2{ PlaceType::Module, builder.addPlace(SpecTypeValueHolder<IntValue>::specTypeId) };
    const PlaceData placeLit{ PlaceType::Module, builder.addPlace(makeValue(std::string{"111222333444"})) };
    const PlaceData placeLit2{ PlaceType::Module, builder.addPlace(makeValue(98765_I)) };
    CodeBlock code{ { builder.build() } };
    FunctionContext funcCont{"int", {}, place1, {placeLit}, {}, &code, &code.getDataLayout()};
    code.operations.push_back(testModule.buildFunction(funcCont));
    funcCont.name = "copy";
    funcCont.returnPlace = place2;
    funcCont.argPlaces = {placeLit2};
    code.operations.push_back(testModule.buildFunction(funcCont));
    funcCont.name = "+=";
    funcCont.returnPlace = {PlaceType::Void, 0};
    funcCont.argPlaces = {place2, place1};
    code.operations.push_back(testModule.buildFunction(funcCont));
    DataBlock<> data { code.getDataLayout() };

    ExecutionContext execCont;
    execCont.run(code, data);

    EXPECT_EQ(static_cast<TypeValueHolder<IntValue>&>(execCont.get(place1)).get(), 111222333444_I);
    EXPECT_EQ(static_cast<TypeValueHolder<IntValue>&>(execCont.get(place2)).get(), 111222432209_I);
}

TEST_F(CoreModuleFixture, FloatCreateAndAdd)
{
    DataBlockDef::Builder builder;
    const PlaceData place1{ PlaceType::Module, builder.addPlace(SpecTypeValueHolder<FloatValue>::specTypeId) };
    const PlaceData place2{ PlaceType::Module, builder.addPlace(SpecTypeValueHolder<FloatValue>::specTypeId) };
    const PlaceData placeLit{ PlaceType::Module, builder.addPlace(makeValue(std::string{"12.34e5"})) };
    const PlaceData placeLit2{ PlaceType::Module, builder.addPlace(makeValue(FloatValue{-78901.23})) };
    CodeBlock code{ { builder.build() } };
    FunctionContext funcCont{"float", {}, place1, {placeLit}, {}, &code, &code.getDataLayout()};
    code.operations.push_back(testModule.buildFunction(funcCont));
    funcCont.name = "copy";
    funcCont.returnPlace = place2;
    funcCont.argPlaces = {placeLit2};
    code.operations.push_back(testModule.buildFunction(funcCont));
    funcCont.name = "+=";
    funcCont.returnPlace = {PlaceType::Void, 0};
    funcCont.argPlaces = {place2, place1};
    code.operations.push_back(testModule.buildFunction(funcCont));
    DataBlock<> data { code.getDataLayout() };

    ExecutionContext execCont;
    execCont.run(code, data);

    EXPECT_EQ(static_cast<TypeValueHolder<FloatValue>&>(execCont.get(place1)).get(), 12.34e5);
    EXPECT_EQ(static_cast<TypeValueHolder<FloatValue>&>(execCont.get(place2)).get(), 12.34e5 - 78901.23);
}

TEST_F(CoreModuleFixture, Fibonacci)
{
    ExecutionContext context;
    const IntValue count1{50_I};
    auto [code, countVal, resultVal] = createFibonacci(context, count1);
    DataBlock<> data { code.getDataLayout() };

    context.run(code, data);
    EXPECT_EQ(*resultVal, 12586269025_I);

    *countVal = 89_I;
    context.run(code, data);
    EXPECT_EQ(*resultVal, 1779979416004714189_I);

    *countVal = 200_I;
    context.run(code, data);
    EXPECT_EQ(*resultVal, 280571172992510140037611932413038677189525_I);
}

TEST_F(CoreModuleFixture, Fibonacci_Cache)
{
    ExecutionContext context;
    const IntValue count1{50_I};
    auto [code, countVal, resultVal] = createFibonacci(context, count1, {FunctionOptions::Cache});
    DataBlock<> data { code.getDataLayout() };

    context.run(code, data);
    EXPECT_EQ(*resultVal, 12586269025_I);

    *countVal = 89_I;
    context.run(code, data);
    EXPECT_EQ(*resultVal, 1779979416004714189_I);

    *countVal = 200_I;
    context.run(code, data);
    EXPECT_EQ(*resultVal, 280571172992510140037611932413038677189525_I);
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
    auto [code, countVal, resultVal] = createFibonacci(context, GetParam().count, GetParam().options);
    DataBlock<> data { code.getDataLayout() };
    context.run(code, data);
    EXPECT_EQ(*resultVal, GetParam().result);

	for (int i = 0; i < repeats; ++i)
    {
        context.run(code, data);
    }
    EXPECT_EQ(*resultVal, GetParam().result);
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

Module createLegacyModule()
{
    Module legacyMod{"legacy", {}};
    legacyMod.defFunction("=", &assign)
        .defFunction("+=", &add)
        .defFunction("swap", &swap)
        .defFunction(">", &greater);
    return legacyMod;
}

std::tuple<CodeBlock, PlaceData> createFibonacciOld(const Module& legacyMod, const long long count)
{
    DataBlockDef::Builder builder;
    const PlaceData placeStart0{ PlaceType::Module, builder.addPlace(makeValue(long long(0))) };
    const PlaceData placeStart1{ PlaceType::Module, builder.addPlace(makeValue(long long(1))) };
    const PlaceData placeStart2{ PlaceType::Module, builder.addPlace(makeValue(long long(-1))) };
    const PlaceData placeCountConst{ PlaceType::Module, builder.addPlace(makeValue(count)) };
    const PlaceData placeCount{ PlaceType::Module, builder.addPlace(SpecTypeValueHolder<long long>::specTypeId) };
    const PlaceData placeFirst{ PlaceType::Module, builder.addPlace(SpecTypeValueHolder<long long>::specTypeId) };
    const PlaceData placeSecond{ PlaceType::Module, builder.addPlace(SpecTypeValueHolder<long long>::specTypeId) };
    CodeBlock code{ { builder.build() } };
    FunctionContext funcCont{"=", {FunctionOptions::Cache}, placeFirst, {placeStart0}, {}, &code, &code.getDataLayout()};
    code.operations.push_back(legacyMod.buildFunction(funcCont));
    funcCont.returnPlace = placeSecond;
    funcCont.argPlaces = {placeStart1};
    code.operations.push_back(legacyMod.buildFunction(funcCont));
    funcCont.returnPlace = placeCount;
    funcCont.argPlaces = {placeCountConst};
    code.operations.push_back(legacyMod.buildFunction(funcCont));
    funcCont = {"+=", {FunctionOptions::Cache}, {PlaceType::Void}, {placeFirst, placeSecond}, {}, &code, &code.getDataLayout()};
    code.operations.push_back(legacyMod.buildFunction(funcCont));
    funcCont = {"swap", {FunctionOptions::Cache}, {PlaceType::Void}, {placeFirst, placeSecond}, {}, &code, &code.getDataLayout()};
    code.operations.push_back(legacyMod.buildFunction(funcCont));
    funcCont = {"+=", {FunctionOptions::Cache}, {PlaceType::Void}, {placeCount, placeStart2}, {}, &code, &code.getDataLayout()};
    code.operations.push_back(legacyMod.buildFunction(funcCont));
    funcCont = {">", {FunctionOptions::Cache, FunctionOptions::Jump}, {PlaceType::Void}, {placeCount, placeStart1}, {-3, 1}, &code, &code.getDataLayout()};
    code.operations.push_back(legacyMod.buildFunction(funcCont));
    return { std::move(code), placeSecond };
}

TEST_F(CoreModulePerformanceFixture, Fibonacci_OldComparison)
{
    Module legacyMod{ createLegacyModule() };
    auto [code, placeResult] = createFibonacciOld(legacyMod, 90);
    DataBlock<> data { code.getDataLayout() };

    ExecutionContext context;
    context.run(code, data);
    const auto checkResult = fibonacci2<long long>(90);
    const auto& resultRef = static_cast<TypeValueHolder<long long>&>(data.get(placeResult.index));
    EXPECT_EQ(resultRef.get(), checkResult);

	for (size_t i = 0; i < 10000; ++i)
	{
        context.run(code, data);
	}

    EXPECT_EQ(resultRef.get(), checkResult);
}
