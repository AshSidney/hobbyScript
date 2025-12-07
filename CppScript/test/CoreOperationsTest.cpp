#include <gmock/gmock.h>

#include <CppScript/CoreOperations.h>
#include <CppScript/OperationBlock.h>
#include "PerfAlgorithms.h"
#include "TestUtils.h"
#include <tuple>

using namespace CppScript;
using namespace CppScriptTest;

class CoreOperationsFixture : public testing::Test
{
protected:
    template <typename T, typename OB>
    struct CalcContext
    {
        using OpType = OB::OperationType;
        OperationBlock<OB> block;
        OperationBlockContext context;
        std::optional<OperationFrame<OpType, DefaultAllocator>> frame;
        Value<T>* count;
        Value<T>* result;
    };

    template <typename T, typename OB>
    CalcContext<T, OB> createFibonacci(const OB& builder, T count)
    {
        TypeFrames frames;
        OperationBlockResolutionData blockData{{}, makeConstants(count, T{0}, T{1}, T{2}),
            {{{"="}, ValuePlace{ValuePlace::Type::Local, 0}, {{ValuePlace::Type::Constants, 1}}},
            {{"="}, ValuePlace{ValuePlace::Type::Local, 1}, {{ValuePlace::Type::Constants, 2}}},
            {{"="}, ValuePlace{ValuePlace::Type::Local, 2}, {{ValuePlace::Type::Constants, 0}}},
            {{"<=>"}, {}, {{ValuePlace::Type::Local, 2}, {ValuePlace::Type::Constants, 2}}, {5, 4, 1}},
            {{"+="}, {}, {{ValuePlace::Type::Local, 0}, {ValuePlace::Type::Local, 1}}},
            {{"+="}, {}, {{ValuePlace::Type::Local, 1}, {ValuePlace::Type::Local, 0}}},
            {{"-="}, {}, {{ValuePlace::Type::Local, 2}, {ValuePlace::Type::Constants, 3}}, {-3}},
            {{"="}, ValuePlace{ValuePlace::Type::Local, 0}, {{ValuePlace::Type::Local, 1}}}}};
        CalcContext<T, OB> context{ { builder, get<OperationBlockBuildContext>(builder.getResolver().resolve(std::move(blockData), frames)) } };
        context.context = context.block.createContext();
        context.frame.emplace(context.block.createFrame(context.context));
        context.count = static_cast<Value<T>*>(getFrame(context.context.frames, ValuePlace::Type::Constants)[0]);
        context.result = static_cast<Value<T>*>(getFrame(context.context.frames, ValuePlace::Type::Local)[0]);
        return context;
    }


    template <typename T, typename OB>
    CalcContext<T, OB> createFactorial(const OB& builder, T count)
    {
        TypeFrames frames;
        OperationBlockResolutionData blockData{{}, makeConstants(count, T{1}),
            {{{"="}, ValuePlace{ValuePlace::Type::Local, 0}, {{ValuePlace::Type::Constants, 0}}},
            {{"="}, ValuePlace{ValuePlace::Type::Local, 1}, {{ValuePlace::Type::Constants, 0}}},
            {{"-="}, {}, {{ValuePlace::Type::Local, 1}, {ValuePlace::Type::Constants, 1}}},
            {{"<=>"}, {}, {{ValuePlace::Type::Local, 1}, {ValuePlace::Type::Constants, 1}}, {2, 2, 1}},
            {{"*="}, {}, {{ValuePlace::Type::Local, 0}, {ValuePlace::Type::Local, 1}}, {-2}}}};
        CalcContext<T, OB> context{ { builder, get<OperationBlockBuildContext>(builder.getResolver().resolve(std::move(blockData), frames)) } };
        context.context = context.block.createContext();
		context.frame.emplace(context.block.createFrame(context.context));
        context.count = static_cast<Value<T>*>(getFrame(context.context.frames, ValuePlace::Type::Constants)[0]);
        context.result = static_cast<Value<T>*>(getFrame(context.context.frames, ValuePlace::Type::Local)[0]);
        return context;
    }

    OperationBuilder<> makeCustomBuilder()
    {
        OperationBuilder<> builder;
        builder.addCustomOperations<IntConstruct, CopyOperation<IntValue>,
            AddOperation<IntValue>, SubtractOperation<IntValue>, CompareOperation<IntValue, std::strong_ordering>,
            CopyOperation<StdInt>, AddOperation<StdInt>,
            SubtractOperation<StdInt>, CompareOperation<StdInt, std::strong_ordering>>();
        return builder;
    }
};

TEST_F(CoreOperationsFixture, TypeName)
{
    const CoreOperationBuilder builder;
    EXPECT_EQ(Value<IntValue>::typeId.typeName.objectId, "int");
    EXPECT_EQ(Value<IntValue>::typeId.typeName.moduleId, "");
}

TEST_F(CoreOperationsFixture, Fibonacci)
{
    auto fibContext = createFibonacci(CoreOperationBuilder{}, 50_I);
    fibContext.frame->execute();
    EXPECT_EQ(fibContext.result->get(), 12586269025_I);

    fibContext.count->set(89_I);
    fibContext.frame->restart();
    fibContext.frame->execute();
    EXPECT_EQ(fibContext.result->get(), 1779979416004714189_I);

    fibContext.count->set(200_I);
    fibContext.frame->restart();
    fibContext.frame->execute();
    EXPECT_EQ(fibContext.result->get(), 280571172992510140037611932413038677189525_I);
}

TEST_F(CoreOperationsFixture, Factorial)
{
    auto fibContext = createFactorial(CoreOperationBuilder{}, 20_I);
    fibContext.frame->execute();
    EXPECT_EQ(fibContext.result->get(), 2432902008176640000_I);

    fibContext.count->set(50_I);
    fibContext.frame->restart();
    fibContext.frame->execute();
    EXPECT_EQ(fibContext.result->get(), IntValue{ fact50 });

    fibContext.count->set(100_I);
    fibContext.frame->restart();
    fibContext.frame->execute();
    EXPECT_EQ(fibContext.result->get(), IntValue{ fact100 });

    fibContext.count->set(200_I);
    fibContext.frame->restart();
    fibContext.frame->execute();
    EXPECT_EQ(fibContext.result->get(), IntValue{ fact200 });
}


struct CalcParams
{
    IntValue count;
    IntValue result;
};

class CoreOperationsPerformanceFixture : public CoreOperationsFixture, public testing::WithParamInterface<CalcParams>
{
protected:
    const int repeats{10000};
};

class CoreOperationsPerformanceFibonacciFixture : public CoreOperationsPerformanceFixture
{};

TEST_P(CoreOperationsPerformanceFibonacciFixture, FibonacciVariant)
{
    auto fibContext{ createFibonacci(CoreOperationBuilder{}, IntValue{ GetParam().count }) };
    fibContext.frame->execute();
    EXPECT_EQ(fibContext.result->get(), GetParam().result);

	for (int i = 0; i < repeats; ++i)
    {
        fibContext.frame->restart();
        fibContext.frame->execute();
    }
    EXPECT_EQ(fibContext.result->get(), GetParam().result);
}

TEST_P(CoreOperationsPerformanceFibonacciFixture, FibonacciCustom)
{
    auto fibContext{ createFibonacci(makeCustomBuilder(), IntValue{ GetParam().count }) };
    fibContext.frame->execute();
    EXPECT_EQ(fibContext.result->get(), GetParam().result);

	for (int i = 0; i < repeats; ++i)
    {
        fibContext.frame->restart();
        fibContext.frame->execute();
    }
    EXPECT_EQ(fibContext.result->get(), GetParam().result);
}

TEST_P(CoreOperationsPerformanceFibonacciFixture, FibonacciVirtual)
{
    auto fibContext{ createFibonacci(CoreOperationVBuilder{}, IntValue{ GetParam().count }) };
    fibContext.frame->execute();
    EXPECT_EQ(fibContext.result->get(), GetParam().result);

	for (int i = 0; i < repeats; ++i)
    {
        fibContext.frame->restart();
        fibContext.frame->execute();
    }
    EXPECT_EQ(fibContext.result->get(), GetParam().result);
}

TEST_P(CoreOperationsPerformanceFibonacciFixture, FibonacciLambda)
{
    auto fibContext{ createFibonacci(CoreOperationLBuilder{}, IntValue{ GetParam().count }) };
    fibContext.frame->execute();
    EXPECT_EQ(fibContext.result->get(), GetParam().result);

	for (int i = 0; i < repeats; ++i)
    {
        fibContext.frame->restart();
        fibContext.frame->execute();
    }
    EXPECT_EQ(fibContext.result->get(), GetParam().result);
}

INSTANTIATE_TEST_SUITE_P(FibonacciInstances, CoreOperationsPerformanceFibonacciFixture,
    testing::Values(CalcParams{50_I, 12586269025_I},
        CalcParams{51_I, fibonacci2<IntValue>(51)},
        CalcParams{89_I, fibonacci2<IntValue>(89)},
        CalcParams{90_I, fibonacci2<IntValue>(90)},
        CalcParams{200_I, fibonacci2<IntValue>(200)},
        CalcParams{201_I, fibonacci2<IntValue>(201)},
        CalcParams{999_I, fibonacci2<IntValue>(999)},
        CalcParams{1000_I, fibonacci2<IntValue>(1000)}));


class CoreOperationPerformanceStdIntFixture : public CoreOperationsFixture
{
protected:
    const std::size_t repeats{ 10000 };
    const StdInt param{ 90 };
    const StdInt result{ fibonacci2<StdInt>(90) };

    template <typename OB>
    void execute(const OB& builder)
    {
        auto fibContext{ createFibonacci(builder, param) };
        fibContext.frame->execute();
        EXPECT_EQ(fibContext.result->get(), result);

        for (int i = 0; i < repeats; ++i)
        {
            fibContext.frame->restart();
            fibContext.frame->execute();
        }
        EXPECT_EQ(fibContext.result->get(), result);
    }
};

TEST_F(CoreOperationPerformanceStdIntFixture, FibonacciVariant)
{
    execute(CoreOperationBuilder{});
}

TEST_F(CoreOperationPerformanceStdIntFixture, FibonacciCustom)
{
    execute(makeCustomBuilder());
}

TEST_F(CoreOperationPerformanceStdIntFixture, FibonacciVirtual)
{
    execute(CoreOperationVBuilder{});
}

TEST_F(CoreOperationPerformanceStdIntFixture, FibonacciLambda)
{
    execute(CoreOperationLBuilder{});
}


class CoreOperationsPerformanceFactorialFixture : public CoreOperationsPerformanceFixture
{};

TEST_P(CoreOperationsPerformanceFactorialFixture, FactorialVariant)
{
    auto factContext{ createFactorial(CoreOperationBuilder{}, IntValue{ GetParam().count }) };
    factContext.frame->execute();
    EXPECT_EQ(factContext.result->get(), GetParam().result);

	for (int i = 0; i < repeats; ++i)
    {
        factContext.frame->restart();
        factContext.frame->execute();
    }
    EXPECT_EQ(factContext.result->get(), GetParam().result);
}

INSTANTIATE_TEST_SUITE_P(FactorialInstances, CoreOperationsPerformanceFactorialFixture,
    testing::Values(CalcParams{20_I, factorial<IntValue>(20)},
        CalcParams{50_I, factorial<IntValue>(50)},
        CalcParams{100_I, factorial<IntValue>(100)},
        CalcParams{200_I, factorial<IntValue>(200)}));
