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

    std::tuple<CodeBlock, PlaceData> createFibonacci(ExecutionContext& context, FunctionOptions options = FunctionOptions::Default)
    {
        CodeBlock code;
        const auto placeStart0 = code.addPlaceData(TypeValueHolder<IntValue>::typeId);
        const auto placeStart1 = code.addPlaceData(TypeValueHolder<IntValue>::typeId);
        const auto placeStart2 = code.addPlaceData(TypeValueHolder<IntValue>::typeId);
        const auto placeFirst = code.addPlaceData(TypeValueHolder<IntValue>::typeId);
        const auto placeSecond = code.addPlaceData(TypeValueHolder<IntValue>::typeId);
        const auto placeCount = code.addPlaceData(TypeValueHolder<IntValue>::typeId);
        FunctionContext funcCont{"=", FunctionOptions::Default | options, {PlaceType::Void}, {placeFirst, placeStart0}, {}, &code};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont.argPlaces = {placeSecond, placeStart1};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"<=>", FunctionOptions::Jump | options, {PlaceType::Void}, {placeCount, placeStart1}, {5, 6, 1}, &code};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"+=", FunctionOptions::Default | options, {PlaceType::Void}, {placeFirst, placeSecond}, {}, &code};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"+=", FunctionOptions::Default | options, {PlaceType::Void}, {placeSecond, placeFirst}, {}, &code};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"-=", FunctionOptions::Default | options, {PlaceType::Void}, {placeCount, placeStart2}, {}, &code};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"<=>", FunctionOptions::Jump | options, {PlaceType::Void}, {placeCount, placeStart1}, {1, 2, -3}, &code};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"=", FunctionOptions::Default | options, {PlaceType::Void}, {placeSecond, placeFirst}, {}, &code};
        code.operations.push_back(testModule.buildFunction(funcCont));

        static SpecTypeValueHolder<IntValue> start0, start1, start2, first, second;
        start0.set(0_I);
        start1.set(1_I);
        start2.set(2_I);
        first.set(0_I);
        second.set(0_I);

        context.set(placeStart0, start0);
        context.set(placeStart1, start1);
        context.set(placeStart2, start2);
        context.set(placeFirst, first);
        context.set(placeSecond, second);

        return {std::move(code), placeCount};
    }
};

TEST_F(CoreModuleFixture, IntCreateAndAdd)
{
    CodeBlock code;
    const auto placeLocal = code.addPlaceData(TypeValueHolder<IntValue>::typeId);
    const auto placeLit = code.addPlaceData(TypeValueHolder<std::string>::typeId);
    FunctionContext funcCont{"int", FunctionOptions::Default,
        placeLocal, {placeLit}, {}, &code};
    auto makeInt = testModule.buildFunction(funcCont);
    const auto placeLocal2 = code.addPlaceData(TypeValueHolder<IntValue>::typeId);
    funcCont.name = "+=";
    funcCont.returnPlace = {PlaceType::Void, 0};
    funcCont.argPlaces = {placeLocal2, placeLocal};
    auto addInt = testModule.buildFunction(funcCont);

    SpecTypeValueHolder<IntValue> intVal, intVal2;
    SpecTypeValueHolder<std::string> intLit;
    intLit.set("111222333444");
    intVal2.set(98765_I);

    ExecutionContext execCont;
    execCont.set(placeLocal, intVal);
    execCont.set(placeLit, intLit);
    execCont.set(placeLocal2, intVal2);

    makeInt->execute(execCont);
    addInt->execute(execCont);

    EXPECT_EQ(intVal.get(), 111222333444_I);
    EXPECT_EQ(intVal2.get(), 111222432209_I);
}

TEST_F(CoreModuleFixture, FloatCreateAndAdd)
{
    CodeBlock code;
    const auto placeLocal = code.addPlaceData(TypeValueHolder<FloatValue>::typeId);
    const auto placeLit = code.addPlaceData(TypeValueHolder<std::string>::typeId);
    const auto placeFloat = code.addPlaceData(TypeValueHolder<FloatValue>::typeId);
    FunctionContext funcCont{"float", FunctionOptions::Default, placeLocal, {placeLit}, {}, &code};
    auto makeFloat = testModule.buildFunction(funcCont);
    funcCont.name = "+=";
    funcCont.returnPlace = {PlaceType::Void, 0};
    funcCont.argPlaces = {placeFloat, placeLocal};
    auto addFloat = testModule.buildFunction(funcCont);

    SpecTypeValueHolder<FloatValue> floatVal, floatVal2;
    SpecTypeValueHolder<std::string> floatLit;
    floatLit.set("12.34e5");
    floatVal2.set(-78901.23);

    ExecutionContext execCont;
    execCont.set(placeLit, floatLit);
    execCont.set(placeLocal, floatVal);
    execCont.set(placeFloat, floatVal2);

    makeFloat->execute(execCont);
    addFloat->execute(execCont);

    EXPECT_EQ(floatVal.get(), 12.34e5);
    EXPECT_EQ(floatVal2.get(), 12.34e5 - 78901.23);
}

TEST_F(CoreModuleFixture, Fibonacci)
{
    ExecutionContext context;
    auto [code, placeCount] = createFibonacci(context);
    SpecTypeValueHolder<IntValue> count;
    context.set(placeCount, count);
    auto& result = static_cast<TypeValueHolder<IntValue>&>(context.get({PlaceType::Local, 4}));

    count.set(50_I);
    context.run(code);
    EXPECT_EQ(result.get(), 12586269025_I);

    count.set(89_I);
    context.run(code);
    EXPECT_EQ(result.get(), 1779979416004714189_I);

    count.set(200_I);
    context.run(code);
    EXPECT_EQ(result.get(), 280571172992510140037611932413038677189525_I);
}

TEST_F(CoreModuleFixture, Fibonacci_Cache)
{
    ExecutionContext context;
    auto [code, placeCount] = createFibonacci(context, FunctionOptions::Cache);
    SpecTypeValueHolder<IntValue> count;
    context.set(placeCount, count);
    auto& result = static_cast<TypeValueHolder<IntValue>&>(context.get({PlaceType::Local, 4}));

    count.set(50_I);
    context.run(code);
    EXPECT_EQ(result.get(), 12586269025_I);

    count.set(89_I);
    context.run(code);
    EXPECT_EQ(result.get(), 1779979416004714189_I);

    count.set(200_I);
    context.run(code);
    EXPECT_EQ(result.get(), 280571172992510140037611932413038677189525_I);
}


class CoreModulePerformanceFixture : public CoreModuleFixture
{
protected:
    void SetUp(FunctionOptions options)
    {
        PlaceData placeCount;
        std::tie(code, placeCount) = createFibonacci(context, options);
        context.set(placeCount, count);
        result = &static_cast<TypeValueHolder<IntValue>&>(context.get({PlaceType::Local, 4}));
    }

    ExecutionContext context;
    CodeBlock code;
    SpecTypeValueHolder<IntValue> count;
    TypeValueHolder<IntValue>* result;
};

TEST_F(CoreModulePerformanceFixture, Fibonacci_Small_Cache)
{
    SetUp(FunctionOptions::Cache);
	for (int i = 0; i < 100000; ++i)
    {
        count.set(50_I);
        context.run(code);
    }
    EXPECT_EQ(result->get(), 12586269025_I);
}

TEST_F(CoreModulePerformanceFixture, Fibonacci_Middle_Cache)
{
    SetUp(FunctionOptions::Cache);
	for (int i = 0; i < 100000; ++i)
    {
        count.set(200_I);
        context.run(code);
    }
    EXPECT_EQ(result->get(), 280571172992510140037611932413038677189525_I);
}

TEST_F(CoreModulePerformanceFixture, Fibonacci_Big_Cache)
{
    SetUp(FunctionOptions::Cache);
	for (int i = 0; i < 100000; ++i)
    {
        count.set(1000_I);
        context.run(code);
    }
    EXPECT_EQ(result->get(), fibonacci2<IntValue>(1000));
}

TEST_F(CoreModulePerformanceFixture, Fibonacci_Small)
{
    SetUp(FunctionOptions::Default);
	for (int i = 0; i < 100000; ++i)
    {
        count.set(50_I);
        context.run(code);
    }
    EXPECT_EQ(result->get(), 12586269025_I);
}

TEST_F(CoreModulePerformanceFixture, Fibonacci_Middle)
{
    SetUp(FunctionOptions::Default);
	for (int i = 0; i < 100000; ++i)
    {
        count.set(200_I);
        context.run(code);
    }
    EXPECT_EQ(result->get(), 280571172992510140037611932413038677189525_I);
}

TEST_F(CoreModulePerformanceFixture, Fibonacci_Big)
{
    SetUp(FunctionOptions::Default);
	for (int i = 0; i < 100000; ++i)
    {
        count.set(1000_I);
        context.run(code);
    }
    EXPECT_EQ(result->get(), fibonacci2<IntValue>(1000));
}


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

    const auto placeStart0 = code.addPlaceData(TypeValueHolder<long long>::typeId);
    const auto placeStart1 = code.addPlaceData(TypeValueHolder<long long>::typeId);
    const auto placeStart2 = code.addPlaceData(TypeValueHolder<long long>::typeId);
    const auto placeFirst = code.addPlaceData(TypeValueHolder<long long>::typeId);
    const auto placeSecond = code.addPlaceData(TypeValueHolder<long long>::typeId);
    const auto placeCount = code.addPlaceData(TypeValueHolder<long long>::typeId);
    FunctionContext funcCont{"=", FunctionOptions::Cache, placeFirst, {placeStart0}, {}, &code};
    code.operations.push_back(legacyMod.buildFunction(funcCont));
    funcCont.returnPlace = placeSecond;
    funcCont.argPlaces = {placeStart1};
    code.operations.push_back(legacyMod.buildFunction(funcCont));
    funcCont = {"+=", FunctionOptions::Cache, {PlaceType::Void}, {placeFirst, placeSecond}, {}, &code};
    code.operations.push_back(legacyMod.buildFunction(funcCont));
    funcCont = {"swap", FunctionOptions::Cache, {PlaceType::Void}, {placeFirst, placeSecond}, {}, &code};
    code.operations.push_back(legacyMod.buildFunction(funcCont));
    funcCont = {"+=", FunctionOptions::Cache, {PlaceType::Void}, {placeCount, placeStart2}, {}, &code};
    code.operations.push_back(legacyMod.buildFunction(funcCont));
    funcCont = {">", FunctionOptions::Jump | FunctionOptions::Cache, {PlaceType::Void}, {placeCount, placeStart1}, {-3, 1}, &code};
    code.operations.push_back(legacyMod.buildFunction(funcCont));

    static SpecTypeValueHolder<long long> start0, start1, start2, first, second, count;
    start0.set(0);
    start1.set(1);
    start2.set(-1);

    context.set(placeStart0, start0);
    context.set(placeStart1, start1);
    context.set(placeStart2, start2);
    context.set(placeFirst, first);
    context.set(placeSecond, second);
    context.set(placeCount, count);

	for (size_t i = 0; i < 10000; ++i)
	{
        count.set(90);
        context.run(code);
	}

    EXPECT_EQ(second.get(), fibonacci2<long long>(90));
}
