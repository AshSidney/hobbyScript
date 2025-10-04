#include <gmock/gmock.h>

#include <CppScript/CoreOperations.h>
#include <CppScript/OperationBlock.h>
#include "Fibonacci.h"
#include "TestUtils.h"
#include <tuple>

using namespace CppScript;
using namespace CppScriptTest;

class CoreOperationsFixture : public testing::Test
{
protected:
    template <typename T, typename OB>
    struct FibonacciContext
    {
        using OpType = OB::OperationType;
        OperationBlock<OpType> block;
        OperationFrame<OpType, DefaultAllocator> frame;
        Value<T>* count;
        Value<T>* result;
    };

    template <typename T, typename OB>
    FibonacciContext<T, OB> createFibonacci(OB builder)
    {
        const auto& resolver = builder.getResolver();
        const TypeId* typeId{ &Value<T>::typeId };
        std::array<std::size_t, 4> opCodes{ getOpRes(resolver, {"="}, {typeId}).index,
			getOpRes(resolver, {"+="}, {typeId, typeId}).index,
            getOpRes(resolver, {"-="}, {typeId, typeId}).index,
			getOpRes(resolver, {"<=>"}, {typeId, typeId}).index };
        using OpType = typename OB::OperationType;
        OperationBlock<OpType> opBlock{ { typeId, typeId, typeId, typeId, typeId, typeId, typeId },
            { [&builder, &opCodes]()
                {
                    std::vector<OpType> ops;
                    ops.emplace_back(builder.build({opCodes[0], {{ValuePlace::Type::Local, 1}, {ValuePlace::Type::Local, 4}}}));
                    ops.emplace_back(builder.build({opCodes[0], {{ValuePlace::Type::Local, 2}, {ValuePlace::Type::Local, 5}}}));
                    ops.emplace_back(builder.build({opCodes[0], {{ValuePlace::Type::Local, 0}, {ValuePlace::Type::Local, 6}}}));
                    ops.emplace_back(builder.build({opCodes[3], {{ValuePlace::Type::Local, 6}, {ValuePlace::Type::Local, 2}}, {5, 4, 1}}));
                    ops.emplace_back(builder.build({opCodes[1], {{ValuePlace::Type::Local, 4}, {ValuePlace::Type::Local, 5}}}));
                    ops.emplace_back(builder.build({opCodes[1], {{ValuePlace::Type::Local, 5}, {ValuePlace::Type::Local, 4}}}));
                    ops.emplace_back(builder.build({opCodes[2], {{ValuePlace::Type::Local, 6}, {ValuePlace::Type::Local, 3}}, {-3}}));
                    ops.emplace_back(builder.build({opCodes[0], {{ValuePlace::Type::Local, 5}, {ValuePlace::Type::Local, 4}}}));
                    return ops;
                }()
            } };
		OperationBlockContext context;
		auto opFrame{ opBlock.createFrame(context) };
		static_cast<Value<T>&>(opFrame.getValues().get(1)).set(T{0});
		static_cast<Value<T>&>(opFrame.getValues().get(2)).set(T{1});
		static_cast<Value<T>&>(opFrame.getValues().get(3)).set(T{2});
        Value<T>* count = static_cast<Value<T>*>(&opFrame.getValues().get(0));
        Value<T>* result = static_cast<Value<T>*>(&opFrame.getValues().get(4));
        return {std::move(opBlock), std::move(opFrame), count, result};
    }

    OperationBuilder<> makeCustomBuilder()
    {
        OperationBuilder<> builder;
        builder.addCustomOperations<CustomOperationWrapper<IntConstruct>, CustomOperationWrapper<CopyOperation<IntValue>>,
            CustomOperationWrapper<AddOperation<IntValue>>, CustomOperationWrapper<SubtractOperation<IntValue>>, CustomOperationWrapper<CompareOperation<IntValue, std::strong_ordering>>,
            CustomOperationWrapper<CopyOperation<StdInt>>, CustomOperationWrapper<AddOperation<StdInt>>,
            CustomOperationWrapper<SubtractOperation<StdInt>>, CustomOperationWrapper<CompareOperation<StdInt, std::strong_ordering>>>();
        return builder;
    }
};

TEST_F(CoreOperationsFixture, Fibonacci)
{
    FibonacciContext fibContext = createFibonacci<IntValue>(CoreOperationBuilder{});
	fibContext.count->set(50_I);
    fibContext.frame.execute();
    EXPECT_EQ(fibContext.result->get(), 12586269025_I);

    fibContext.count->set(89_I);
    fibContext.frame.restart();
    fibContext.frame.execute();
    EXPECT_EQ(fibContext.result->get(), 1779979416004714189_I);

    fibContext.count->set(200_I);
    fibContext.frame.restart();
    fibContext.frame.execute();
    EXPECT_EQ(fibContext.result->get(), 280571172992510140037611932413038677189525_I);
}

TEST_F(CoreOperationsFixture, TypeName)
{
    const CoreOperationBuilder builder;
    EXPECT_EQ(Value<IntValue>::typeId.typeName.objectId, "int");
    EXPECT_EQ(Value<IntValue>::typeId.typeName.moduleId, "");
}


struct FibonacciParams
{
    IntValue count;
    IntValue result;
};

class CoreOperationsPerformanceFixture : public CoreOperationsFixture, public testing::WithParamInterface<FibonacciParams>
{
protected:
    const int repeats{10000};
};

TEST_P(CoreOperationsPerformanceFixture, FibonacciVariant)
{
    FibonacciContext fibContext{ createFibonacci<IntValue>(CoreOperationBuilder{}) };
    fibContext.count->set(IntValue{ GetParam().count });
    fibContext.frame.execute();
    EXPECT_EQ(fibContext.result->get(), GetParam().result);

	for (int i = 0; i < repeats; ++i)
    {
        fibContext.frame.restart();
        fibContext.frame.execute();
    }
    EXPECT_EQ(fibContext.result->get(), GetParam().result);
}

TEST_P(CoreOperationsPerformanceFixture, FibonacciCustom)
{
    FibonacciContext fibContext{ createFibonacci<IntValue>(makeCustomBuilder()) };
    fibContext.count->set(IntValue{ GetParam().count });
    fibContext.frame.execute();
    EXPECT_EQ(fibContext.result->get(), GetParam().result);

	for (int i = 0; i < repeats; ++i)
    {
        fibContext.frame.restart();
        fibContext.frame.execute();
    }
    EXPECT_EQ(fibContext.result->get(), GetParam().result);
}

TEST_P(CoreOperationsPerformanceFixture, FibonacciVirtual)
{
    FibonacciContext fibContext{ createFibonacci<IntValue>(CoreOperationVBuilder{}) };
    fibContext.count->set(IntValue{ GetParam().count });
    fibContext.frame.execute();
    EXPECT_EQ(fibContext.result->get(), GetParam().result);

	for (int i = 0; i < repeats; ++i)
    {
        fibContext.frame.restart();
        fibContext.frame.execute();
    }
    EXPECT_EQ(fibContext.result->get(), GetParam().result);
}

TEST_P(CoreOperationsPerformanceFixture, FibonacciLambda)
{
    FibonacciContext fibContext{ createFibonacci<IntValue>(CoreOperationLBuilder{}) };
    fibContext.count->set(IntValue{ GetParam().count });
    fibContext.frame.execute();
    EXPECT_EQ(fibContext.result->get(), GetParam().result);

	for (int i = 0; i < repeats; ++i)
    {
        fibContext.frame.restart();
        fibContext.frame.execute();
    }
    EXPECT_EQ(fibContext.result->get(), GetParam().result);
}

INSTANTIATE_TEST_SUITE_P(FibonacciInstances, CoreOperationsPerformanceFixture,
    testing::Values(FibonacciParams{50_I, 12586269025_I},
        FibonacciParams{51_I, fibonacci2<IntValue>(51)},
        FibonacciParams{89_I, fibonacci2<IntValue>(89)},
        FibonacciParams{90_I, fibonacci2<IntValue>(90)},
        FibonacciParams{200_I, fibonacci2<IntValue>(200)},
        FibonacciParams{201_I, fibonacci2<IntValue>(201)},
        FibonacciParams{999_I, fibonacci2<IntValue>(999)},
        FibonacciParams{1000_I, fibonacci2<IntValue>(1000)}));


class CoreOperationPerformanceStdIntFixture : public CoreOperationsFixture
{
protected:
    const std::size_t repeats{ 10000 };
    const StdInt param{ 90 };
    const StdInt result{ fibonacci2<StdInt>(90) };

    template <typename OB>
    void execute(const OB& builder)
    {
        FibonacciContext<StdInt, OB> fibContext{ createFibonacci<StdInt>(builder) };
        fibContext.count->set(StdInt{param});
        fibContext.frame.execute();
        EXPECT_EQ(fibContext.result->get(), result);

        for (int i = 0; i < repeats; ++i)
        {
            fibContext.frame.restart();
            fibContext.frame.execute();
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
