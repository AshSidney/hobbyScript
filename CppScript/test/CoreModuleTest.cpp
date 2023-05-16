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

    CodeBlock createFibonacci(ExecutionContext& context, FunctionOptions options = FunctionOptions::Default)
    {
        CodeBlock code;
        FunctionContext funcCont{"=", FunctionOptions::Default | options, {PlaceType::Void}, {{PlaceType::Local, 3}, {PlaceType::Local, 0}}, {}, &code};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont.argPlaces = {{PlaceType::Local, 4}, {PlaceType::Local, 1}};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"<=>", FunctionOptions::Jump | options, {PlaceType::Void}, {{PlaceType::Local, 5}, {PlaceType::Local, 1}}, {5, 6, 1}, &code};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"+=", FunctionOptions::Default | options, {PlaceType::Void}, {{PlaceType::Local, 3}, {PlaceType::Local, 4}}, {}, &code};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"+=", FunctionOptions::Default | options, {PlaceType::Void}, {{PlaceType::Local, 4}, {PlaceType::Local, 3}}, {}, &code};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"-=", FunctionOptions::Default | options, {PlaceType::Void}, {{PlaceType::Local, 5}, {PlaceType::Local, 2}}, {}, &code};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"<=>", FunctionOptions::Jump | options, {PlaceType::Void}, {{PlaceType::Local, 5}, {PlaceType::Local, 1}}, {1, 2, -3}, &code};
        code.operations.push_back(testModule.buildFunction(funcCont));
        funcCont = {"=", FunctionOptions::Default | options, {PlaceType::Void}, {{PlaceType::Local, 4}, {PlaceType::Local, 3}}, {}, &code};
        code.operations.push_back(testModule.buildFunction(funcCont));

        static SpecTypeValueHolder<IntValue> start0, start1, start2, first, second;
        start0.set(0_I);
        start1.set(1_I);
        start2.set(2_I);
        first.set(0_I);
        second.set(0_I);

        context.set({PlaceType::Local, 0}, start0);
        context.set({PlaceType::Local, 1}, start1);
        context.set({PlaceType::Local, 2}, start2);
        context.set({PlaceType::Local, 3}, first);
        context.set({PlaceType::Local, 4}, second);

        return code;
    }
};

TEST_F(CoreModuleFixture, CreateIntAndAdd)
{
    FunctionContext funcCont{"int", FunctionOptions::Default,
        {PlaceType::Local, 0}, {{PlaceType::Local, 1}}};
    auto makeInt = testModule.buildFunction(funcCont);
    funcCont.name = "+=";
    funcCont.returnPlace = {PlaceType::Void, 0};
    funcCont.argPlaces = {{PlaceType::Local, 2}, {PlaceType::Local, 0}};
    auto addInt = testModule.buildFunction(funcCont);

    SpecTypeValueHolder<IntValue> intVal, intVal2;
    SpecTypeValueHolder<std::string> intLit;
    intLit.set("111222333444");
    intVal2.set(98765_I);

    ExecutionContext execCont;
    execCont.set({PlaceType::Local, 0}, intVal);
    execCont.set({PlaceType::Local, 1}, intLit);
    execCont.set({PlaceType::Local, 2}, intVal2);

    makeInt->execute(execCont);
    addInt->execute(execCont);

    EXPECT_EQ(intVal.get(), 111222333444_I);
    EXPECT_EQ(intVal2.get(), 111222432209_I);
}

TEST_F(CoreModuleFixture, Fibonacci)
{
    ExecutionContext context;
    CodeBlock code = createFibonacci(context);
    SpecTypeValueHolder<IntValue> count;
    context.set({PlaceType::Local, 5}, count);
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
    CodeBlock code = createFibonacci(context, FunctionOptions::Cache);
    SpecTypeValueHolder<IntValue> count;
    context.set({PlaceType::Local, 5}, count);
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
        code = createFibonacci(context, options);
        context.set({PlaceType::Local, 5}, count);
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

    FunctionContext funcCont{"=", FunctionOptions::Cache, {PlaceType::Local, 3}, {{PlaceType::Local, 0}}, {}, &code};
    code.operations.push_back(legacyMod.buildFunction(funcCont));
    funcCont.returnPlace = {PlaceType::Local, 4};
    funcCont.argPlaces = {{PlaceType::Local, 1}};
    code.operations.push_back(legacyMod.buildFunction(funcCont));
    funcCont = {"+=", FunctionOptions::Cache, {PlaceType::Void}, {{PlaceType::Local, 3}, {PlaceType::Local, 4}}, {}, &code};
    code.operations.push_back(legacyMod.buildFunction(funcCont));
    funcCont = {"swap", FunctionOptions::Cache, {PlaceType::Void}, {{PlaceType::Local, 3}, {PlaceType::Local, 4}}, {}, &code};
    code.operations.push_back(legacyMod.buildFunction(funcCont));
    funcCont = {"+=", FunctionOptions::Cache, {PlaceType::Void}, {{PlaceType::Local, 5}, {PlaceType::Local, 2}}, {}, &code};
    code.operations.push_back(legacyMod.buildFunction(funcCont));
    funcCont = {">", FunctionOptions::Jump | FunctionOptions::Cache, {PlaceType::Void}, {{PlaceType::Local, 5}, {PlaceType::Local, 1}}, {-3, 1}, &code};
    code.operations.push_back(legacyMod.buildFunction(funcCont));

    static SpecTypeValueHolder<long long> start0, start1, start2, first, second, count;
    start0.set(0);
    start1.set(1);
    start2.set(-1);

    context.set({PlaceType::Local, 0}, start0);
    context.set({PlaceType::Local, 1}, start1);
    context.set({PlaceType::Local, 2}, start2);
    context.set({PlaceType::Local, 3}, first);
    context.set({PlaceType::Local, 4}, second);
    context.set({PlaceType::Local, 5}, count);

	for (size_t i = 0; i < 10000; ++i)
	{
        count.set(90);
        context.run(code);
	}

    EXPECT_EQ(second.get(), fibonacci2<long long>(90));
}
