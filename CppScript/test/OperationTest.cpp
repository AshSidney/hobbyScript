#include <gmock/gmock.h>

#include <CppScript/Operation.h>
#include <CppScript/IntValue.h>
#include "TestUtils.h"
#include <array>
#include <iterator>

using namespace CppScript;
using namespace CppScriptTest;

class TestOpAdd : public OperationHelper<TestOpAdd, int&, int&, const int>
{
public:
    int& operator()(int& l, const int r) const
    {
        l += r;
        return l;
    }

    constexpr static OperationId id{ "+=" };
};

class TestOpAddFloat : public OperationHelper<TestOpAddFloat, float, const float, const float>
{
public:
    float operator()(const float l, const float r) const
    {
        return l + r;
    }

    constexpr static OperationId id{ "+=" };
};

struct TestVec
{
    std::array<float, 3> vec{ 0.0F, 0.0F, 0.0F };
    bool normed{ false };
};

class TestOpSetVec : public OperationHelper<TestOpSetVec, void, TestVec&, const float[3]>
{
public:
    void operator()(TestVec& vec, const float val[3]) const
    {
        std::copy(val, val + 3, vec.vec.begin());
        vec.normed = false;
    }

    constexpr static OperationId id{ "setVec", "testVec" };
};

class TestOpVecNorm : public OperationHelper<TestOpVecNorm, void, TestVec&>
{
public:
    void operator()(TestVec& vec) const
    {
        float lenght = std::sqrt(vec.vec[0] * vec.vec[0] + vec.vec[1] * vec.vec[1] + vec.vec[2] * vec.vec[2]);
        vec.normed = lenght > 0.0F;
        if (vec.normed)
            for (auto& coord : vec.vec)
                coord /= lenght;
    }

    constexpr static OperationId id{ "norm", "testVec" };
};

class TestOpFloatCompare : public OperationHelper<TestOpFloatCompare, std::partial_ordering, const float, const float>
{
public:
    std::partial_ordering operator()(const float l, const float r) const
    {
        return l <=> r;
    }

    constexpr static OperationId id{ "<=>" };
};

class TestOpFloatMult : public OperationHelper<TestOpFloatMult, void, float&, const float>
{
public:
    void operator()(float& l, const float r) const
    {
        l *= r;
    }

    static constexpr OperationId id{ "*=" };
};

using TestCustomOpFloatMult = CustomOperationWrapper<TestOpFloatMult>;


using TestOperationBuilder = OperationBuilder<TestOpAdd, TestOpAddFloat, TestOpSetVec, TestOpVecNorm, TestOpFloatCompare>;

using TestOperationBuilder1 = OperationBuilder<TestOpAdd, TestOpAddFloat>;

using TestOperationBuilderExtended = OperationBuilderExtension<TestOperationBuilder1, TestOpSetVec, TestOpVecNorm, TestOpFloatCompare>::Type;

static_assert(std::is_same_v<TestOperationBuilder, TestOperationBuilderExtended>);

using TestOperationBuilder0 = OperationBuilder<TestOpAdd>;

static_assert(std::is_same_v<TestOperationBuilder1, OperationBuilderExtension<TestOperationBuilder0, TestOpAddFloat>::Type>);

static_assert(std::is_same_v<OperationBuilderExtension<TestOperationBuilder0, TestOpAddFloat, TestOpFloatCompare>::Type,
    OperationBuilderExtension<TestOperationBuilder1, TestOpFloatCompare>::Type>);


TEST(OperationTest, IntAdd)
{
    Value<int> valInt1, valInt2;
    valInt1.set(45);
    valInt2.set(678);

    TestOperationBuilder builder;
    const auto opRes = getOpRes(builder.getResolver(), Id{"+="}, {&Value<int>::typeId, &Value<int>::typeId});
    EXPECT_EQ(opRes.index, 0);
    EXPECT_EQ(opRes.description.id, Id{"+="});
    EXPECT_EQ(*opRes.description.returnType, Value<int&>::typeId);
    EXPECT_THAT(span2Vector(opRes.description.argumentTypes),
        testing::ElementsAre(&Value<int&>::typeId, &Value<const int>::typeId));
    EXPECT_EQ(opRes.description.jumpCount, 1);
    const auto opAdd = builder.build({opRes.index, {{ValuePlace::Type::Local, 0}, {ValuePlace::Type::Local, 1}, {ValuePlace::Type::Local, 2}}});
    EXPECT_THAT(opAdd.getArgumentPlaces(), testing::ElementsAre(ValuePlace{ValuePlace::Type::Local, 0},
        ValuePlace{ValuePlace::Type::Local, 1}, ValuePlace{ValuePlace::Type::Local, 2}));

    std::array<ValueBase*, 3> args1{ &valInt1, &valInt2, nullptr };
    OperationContext opCont1{args1.data()};
    opAdd.execute(opCont1);
    EXPECT_EQ(opCont1.nextStep, 1);
    EXPECT_EQ(valInt1.get(), 723);
    EXPECT_EQ(valInt2.get(), 678);

    Value<int&> refInt;
    std::array<ValueBase*, 3> args2{ &valInt2, &valInt1, &refInt };
    OperationContext opCont2{args2.data()};
    opAdd.execute(opCont2);
    EXPECT_EQ(opCont2.nextStep, 1);
    EXPECT_EQ(valInt1.get(), 723);
    EXPECT_EQ(valInt2.get(), 1401);
    EXPECT_EQ(refInt.get(), 1401);
}

TEST(OperationTest, FloatAdd)
{
    Value<const float> valFloat1, valFloat2;
    valFloat1.set(45.67F);
    valFloat2.set(9.87F);
    Value<float> valFloat3;

    TestOperationBuilder builder;
    const auto opRes = getOpRes(builder.getResolver(), Id{"+="}, {&Value<float>::typeId, &Value<float>::typeId});
    EXPECT_EQ(opRes.index, 1);
    EXPECT_EQ(opRes.description.id, Id{"+="});
    EXPECT_EQ(*opRes.description.returnType, Value<float>::typeId);
    EXPECT_THAT(span2Vector(opRes.description.argumentTypes),
        testing::ElementsAre(&Value<const float>::typeId, &Value<const float>::typeId));
    EXPECT_EQ(opRes.description.jumpCount, 1);
    const auto opAdd = builder.build({opRes.index, {{ValuePlace::Type::Void}, {ValuePlace::Type::Module, 3}, {ValuePlace::Type::Local, 2}}, {-5}});
    EXPECT_THAT(opAdd.getArgumentPlaces(), testing::ElementsAre(ValuePlace{ValuePlace::Type::Void},
        ValuePlace{ValuePlace::Type::Module, 3}, ValuePlace{ValuePlace::Type::Local, 2}));

    std::array<ValueBase*, 3> args1{ &valFloat1, &valFloat2, &valFloat3 };
    OperationContext opCont1{args1.data()};
    opAdd.execute(opCont1);
    EXPECT_EQ(opCont1.nextStep, -5);
    const float eps = 1E-4F;
    EXPECT_NEAR(valFloat1.get(), 45.67F, eps);
    EXPECT_NEAR(valFloat2.get(), 9.87F, eps);
    EXPECT_NEAR(valFloat3.get(), 55.54F, eps);

    std::array<ValueBase*, 3> args2{ &valFloat3, &valFloat2, &valFloat1 };
    OperationContext opCont2{args2.data()};
    opAdd.execute(opCont2);
    EXPECT_EQ(opCont2.nextStep, -5);
    EXPECT_NEAR(valFloat1.get(), 65.41F, eps);
    EXPECT_NEAR(valFloat2.get(), 9.87F, eps);
    EXPECT_NEAR(valFloat3.get(), 55.54F, eps);
}

TEST(OperationTest, SetVec)
{
    TestVec vecSrc;
    Value<TestVec&> vec;
    vec.set(vecSrc);
    Value<const float[3]> vecVal;
    float vecData[3]{ 2, 3, 1 };
    vecVal.set(vecData);

    TestOperationBuilder builder;
    const auto opRes = getOpRes(builder.getResolver(), Id{"setVec", "testVec"}, {&Value<TestVec>::typeId, &Value<float[3]>::typeId});
    EXPECT_EQ(opRes.index, 2);
    EXPECT_EQ(opRes.description.id, (Id{"setVec", "testVec"}));
    EXPECT_EQ(opRes.description.returnType, nullptr);
    EXPECT_THAT(span2Vector(opRes.description.argumentTypes),
        testing::ElementsAre(&Value<TestVec&>::typeId, &Value<const float[3]>::typeId));
    EXPECT_EQ(opRes.description.jumpCount, 1);
    const auto setVecOp = builder.build({opRes.index, {{ValuePlace::Type::Module, 1}, {ValuePlace::Type::Local, 0}}});
    EXPECT_THAT(setVecOp.getArgumentPlaces(), testing::ElementsAre(ValuePlace{ValuePlace::Type::Module, 1},
        ValuePlace{ValuePlace::Type::Local, 0}));

    std::array<ValueBase*, 2> args{&vec, &vecVal};
    OperationContext opCont{args.data()};
    setVecOp.execute(opCont);
    EXPECT_EQ(opCont.nextStep, 1);
    EXPECT_EQ(vec.get().vec[0], 2);
    EXPECT_EQ(vec.get().vec[1], 3);
    EXPECT_EQ(vec.get().vec[2], 1);
    EXPECT_FALSE(vec.get().normed);

    vecData[0] = 4;
    vecData[2] = 0;
    setVecOp.execute(opCont);
    EXPECT_EQ(vec.get().vec[0], 4);
    EXPECT_EQ(vec.get().vec[1], 3);
    EXPECT_EQ(vec.get().vec[2], 0);
    EXPECT_FALSE(vec.get().normed);
}

TEST(OperationTest, NormVec)
{
    TestVec vecSrc;
    Value<TestVec&> vec;
    vec.set(vecSrc);
    TestOperationBuilder builder;
    const auto opRes = getOpRes(builder.getResolver(), Id{"norm", "testVec"}, {&Value<TestVec>::typeId});
    EXPECT_EQ(opRes.index, 3);
    EXPECT_EQ(opRes.description.id, (Id{"norm", "testVec"}));
    EXPECT_EQ(opRes.description.returnType, nullptr);
    EXPECT_THAT(span2Vector(opRes.description.argumentTypes), testing::ElementsAre(&Value<TestVec&>::typeId));
    EXPECT_EQ(opRes.description.jumpCount, 1);
    const auto normVecOp = builder.build({opRes.index, {{ValuePlace::Type::Module, 4}}});
    EXPECT_THAT(normVecOp.getArgumentPlaces(), testing::ElementsAre(ValuePlace{ValuePlace::Type::Module, 4}));

    std::array<ValueBase*, 1> args{&vec};
    OperationContext opCont{args.data()};
    normVecOp.execute(opCont);
    EXPECT_EQ(opCont.nextStep, 1);
    EXPECT_EQ(vec.get().vec[0], 0);
    EXPECT_EQ(vec.get().vec[1], 0);
    EXPECT_EQ(vec.get().vec[2], 0);
    EXPECT_FALSE(vec.get().normed);

    vecSrc.vec[0] = 4;
    vecSrc.vec[2] = 3;
    normVecOp.execute(opCont);
    const float eps = 1E-6F;
    EXPECT_NEAR(vec.get().vec[0], 0.8F, eps);
    EXPECT_NEAR(vec.get().vec[1], 0, eps);
    EXPECT_NEAR(vec.get().vec[2], 0.6F, eps);
    EXPECT_TRUE(vec.get().normed);
}

TEST(OperationTest, FloatCompareJump)
{
    Value<const float> valFloat1, valFloat2;
    valFloat1.set(45.67F);
    valFloat2.set(9.87F);

    TestOperationBuilder builder;
    const auto opRes = getOpRes(builder.getResolver(), Id{"<=>"}, {&Value<const float>::typeId, &Value<const float>::typeId});
    EXPECT_EQ(opRes.index, 4);
    EXPECT_EQ(opRes.description.id, Id{"<=>"});
    EXPECT_EQ(*opRes.description.returnType, Value<std::partial_ordering>::typeId);
    EXPECT_THAT(span2Vector(opRes.description.argumentTypes),
        testing::ElementsAre(&Value<const float>::typeId, &Value<const float>::typeId));
    EXPECT_EQ(opRes.description.jumpCount, 3);
    const auto opCompare = builder.build({opRes.index, {{ValuePlace::Type::Module, 5}, {ValuePlace::Type::Local, 6}}, {-2, 7, 3}});
    EXPECT_THAT(opCompare.getArgumentPlaces(), testing::ElementsAre(ValuePlace{ValuePlace::Type::Module, 5},
        ValuePlace{ValuePlace::Type::Local, 6}));

    std::array<ValueBase*, 3> args{ &valFloat1, &valFloat2, nullptr };
    OperationContext opCont{args.data()};
    opCompare.execute(opCont);
    EXPECT_EQ(opCont.nextStep, 3);

    valFloat2.set(45.67F);
    opCompare.execute(opCont);
    EXPECT_EQ(opCont.nextStep, 7);

    valFloat2.set(100.0F);
    opCompare.execute(opCont);
    EXPECT_EQ(opCont.nextStep, -2);
}

TEST(OperationTest, GetArgumentPlaces)
{
    const std::vector<TestOperationBuilder::OperationType> operations{ []()
        {
            const TestOperationBuilder builder;
            std::vector<TestOperationBuilder::OperationType> ops;
            ops.push_back(builder.build({0, {{ValuePlace::Type::Local, 0}, {ValuePlace::Type::Module, 1}, {ValuePlace::Type::Local, 2}}, {1} }));
            ops.push_back(builder.build({1, {{ValuePlace::Type::Module, 3}, {ValuePlace::Type::Local, 4}, {ValuePlace::Type::Void}}, {-1}}));
            ops.push_back(builder.build({4, {{ValuePlace::Type::Local, 8}, {ValuePlace::Type::Local, 2}}, {2, -3, 5}}));
            ops.push_back(builder.build({2, {{ValuePlace::Type::Module, 5}, {ValuePlace::Type::Module, 6}}, {}}));
            ops.push_back(builder.build({4, {{ValuePlace::Type::Local, 1}, {ValuePlace::Type::Module, 0}}, {1, 3, -2}})),
            ops.push_back(builder.build({3, {{ValuePlace::Type::Module, 3}}, {}}));
            return ops;
        }() };
    const auto argValues = getArgumentPlaces(operations);
    EXPECT_EQ(argValues.size(), 6);
    EXPECT_THAT(*argValues[0], testing::ElementsAre(ValuePlace{ValuePlace::Type::Local, 0},
        ValuePlace{ValuePlace::Type::Module, 1}, ValuePlace{ValuePlace::Type::Local, 2}));
    EXPECT_THAT(*argValues[1], testing::ElementsAre(ValuePlace{ValuePlace::Type::Module, 3},
        ValuePlace{ValuePlace::Type::Local, 4}, ValuePlace{ValuePlace::Type::Void}));
    EXPECT_THAT(*argValues[2], testing::ElementsAre(ValuePlace{ValuePlace::Type::Local, 8}, ValuePlace{ValuePlace::Type::Local, 2}));
    EXPECT_THAT(*argValues[3], testing::ElementsAre(ValuePlace{ValuePlace::Type::Module, 5},
        ValuePlace{ValuePlace::Type::Module, 6}));
    EXPECT_THAT(*argValues[4], testing::ElementsAre(ValuePlace{ValuePlace::Type::Local, 1}, ValuePlace{ValuePlace::Type::Module, 0}));
    EXPECT_THAT(*argValues[5], testing::ElementsAre(ValuePlace{ValuePlace::Type::Module, 3}));
}


class CustomOperationMock : public CustomOperation::Base
{
public:
    CustomOperationMock(Id id = {})
    {
        mocks.push_back({ this, id });
    }

    ~CustomOperationMock()
    {
        mocks.erase(std::find_if(mocks.begin(), mocks.end(), [this](const MockData& mock){ return this == mock.mock; }));
    }

	MOCK_METHOD(void, execute, (OperationContext& context), (const, override));

    void initialize(OperationBuildContext& context)
    {
        jumps = context.jumps;
    }

    static std::vector<CustomOperationMock*> getMocks(Id id)
    {
        std::vector<CustomOperationMock*> result;
        for (const MockData& mock : mocks)
            if (mock.id == id)
                result.push_back(mock.mock);
        return result;
    }

    struct MockData
    {
        CustomOperationMock* mock;
        Id id;
    };

    std::vector<int> jumps;

    static std::vector<MockData> mocks;
};

std::vector<CustomOperationMock::MockData> CustomOperationMock::mocks;


TEST(OperationTest, CustomOperation)
{
    CustomOperation customOp{ std::make_unique<CustomOperationMock>() };
    CustomOperationMock* opMock = CustomOperationMock::getMocks({}).front();
    OperationContext context;
    EXPECT_CALL(*opMock, execute(testing::Ref(context)))
        .WillOnce([](OperationContext& ctx){ ctx.nextStep = 3; });
    customOp.execute(context);
    EXPECT_EQ(context.nextStep, 3);
    EXPECT_CALL(*opMock, execute(testing::Ref(context)))
        .WillOnce([](OperationContext& ctx){ ctx.nextStep = -2; });
    customOp.execute(context);
    EXPECT_EQ(context.nextStep, -2);
}


class CustomOperationMock1 : public CustomOperationMock
{
public:
    CustomOperationMock1() : CustomOperationMock(getDescription().id){}
    static OperationDescription getDescription()
    {
        static std::vector<const TypeId*> argTypes{ &Value<const float>::typeId, &Value<TestVec*>::typeId };
        return { {"firstOp"}, &Value<float>::typeId, { argTypes.begin(), argTypes.end() }, 1 };
    }
};

class CustomOperationMock2 : public CustomOperationMock
{
public:
    CustomOperationMock2() : CustomOperationMock(getDescription().id){}
    static OperationDescription getDescription()
    {
        static std::vector<const TypeId*> argTypes{ &Value<int&>::typeId };
        return { { "secOp", "testModule" }, nullptr, { argTypes.begin(), argTypes.end() }, 2 };
    }
};

TEST(OperationTest, CustomOperationsAddMockClasses)
{
    TestOperationBuilder0 builder;
    OperationResolver& resolver = builder.getResolver();
    ASSERT_EQ(resolver.getCount(), 1);
    builder.addCustomOperations<CustomOperationMock1, CustomOperationMock2>();
    ASSERT_EQ(resolver.getCount(), 3);

    const auto opRes0 = getOpRes(resolver, {"+="}, {&Value<int&>::typeId, &Value<const int>::typeId});
    EXPECT_EQ(opRes0.index, 0);
    EXPECT_EQ(opRes0.description.id, Id{"+="});
    EXPECT_EQ(*opRes0.description.returnType, Value<int&>::typeId);
    EXPECT_THAT(span2Vector(opRes0.description.argumentTypes),
        testing::ElementsAre(&Value<int&>::typeId, &Value<const int>::typeId));
    EXPECT_EQ(opRes0.description.jumpCount, 1);
    const auto opAdd = builder.build({opRes0.index, {{ValuePlace::Type::Module, 1}, {ValuePlace::Type::Caller, 2}, {ValuePlace::Type::Void}}});
    EXPECT_THAT(opAdd.getArgumentPlaces(), testing::ElementsAre(ValuePlace{ValuePlace::Type::Module, 1},
        ValuePlace{ValuePlace::Type::Caller, 2}, ValuePlace{ValuePlace::Type::Void}));
    Value<int> valInt1, valInt2;
    valInt1.set(123);
    valInt2.set(789);
    std::array<ValueBase*, 3> args0{ &valInt1, &valInt2, nullptr };
    OperationContext opCont1{args0.data()};
    opAdd.execute(opCont1);
    EXPECT_EQ(opCont1.nextStep, 1);
    EXPECT_EQ(valInt1.get(), 912);
    EXPECT_EQ(valInt2.get(), 789);

    const auto opRes1 = getOpRes(resolver, {"firstOp"}, {&Value<const float>::typeId, &Value<TestVec*>::typeId});
    EXPECT_EQ(opRes1.index, 1);
    EXPECT_EQ(opRes1.description.id, Id{"firstOp"});
    EXPECT_EQ(*opRes1.description.returnType, Value<float>::typeId);
    EXPECT_THAT(span2Vector(opRes1.description.argumentTypes),
        testing::ElementsAre(&Value<const float>::typeId, &Value<TestVec*>::typeId));
    EXPECT_EQ(opRes1.description.jumpCount, 1);
    const auto opFirst = builder.build({opRes1.index, {{ValuePlace::Type::Local, 3}, {ValuePlace::Type::Caller, 1}, {ValuePlace::Type::Module, 0}}, {4}});
    EXPECT_THAT(opFirst.getArgumentPlaces(), testing::ElementsAre(ValuePlace{ValuePlace::Type::Local, 3},
        ValuePlace{ValuePlace::Type::Caller, 1}, ValuePlace{ValuePlace::Type::Module, 0}));
    auto* firstMock = CustomOperationMock::getMocks({"firstOp"}).front();
    EXPECT_THAT(firstMock->jumps, testing::ElementsAre(4));

    OperationContext context;
    EXPECT_CALL(*firstMock, execute(testing::Ref(context)))
        .WillOnce([](OperationContext& ctx){ ctx.nextStep = 6; });
    opFirst.execute(context);
    EXPECT_EQ(context.nextStep, 6);

    const auto opRes2 = getOpRes(resolver, {"secOp", "testModule"}, {&Value<int&>::typeId});
    EXPECT_EQ(opRes2.index, 2);
    EXPECT_EQ(opRes2.description.id, (Id{"secOp", "testModule"}));
    EXPECT_EQ(opRes2.description.returnType, nullptr);
    EXPECT_THAT(span2Vector(opRes2.description.argumentTypes), testing::ElementsAre(&Value<int&>::typeId));
    EXPECT_EQ(opRes2.description.jumpCount, 2);
    const auto opSec = builder.build({opRes2.index, {{ValuePlace::Type::Caller, 2}}, {-5, 1}});
    EXPECT_THAT(opSec.getArgumentPlaces(), testing::ElementsAre(ValuePlace{ValuePlace::Type::Caller, 2}));
    auto* secMock = CustomOperationMock::getMocks({"secOp", "testModule"}).front();
    EXPECT_THAT(secMock->jumps, testing::ElementsAre(-5, 1));

    EXPECT_CALL(*secMock, execute(testing::Ref(context)))
        .WillOnce([](OperationContext& ctx){ ctx.nextStep = -8; });
    opSec.execute(context);
    EXPECT_EQ(context.nextStep, -8);

    EXPECT_CALL(*firstMock, execute(testing::Ref(context)))
        .WillOnce([](OperationContext& ctx){ ctx.nextStep = 2; });
    opFirst.execute(context);
    EXPECT_EQ(context.nextStep, 2);
}

TEST(OperationTest, CustomOperationsAddTestClass)
{
    TestOperationBuilder1 builder;
    OperationResolver& resolver = builder.getResolver();
    ASSERT_EQ(resolver.getCount(), 2);
    builder.addCustomOperations<TestCustomOpFloatMult>();
    ASSERT_EQ(resolver.getCount(), 3);
    const auto opRes = getOpRes(resolver, {"*="}, {&Value<float&>::typeId, &Value<const float>::typeId});
    EXPECT_EQ(opRes.index, 2);
    EXPECT_EQ(opRes.description.id, Id{"*="});
    EXPECT_EQ(opRes.description.returnType, nullptr);
    EXPECT_THAT(span2Vector(opRes.description.argumentTypes),
        testing::ElementsAre(&Value<float&>::typeId, &Value<const float>::typeId));
    EXPECT_EQ(opRes.description.jumpCount, 1);

    const auto opMult = builder.build({opRes.index, {{ValuePlace::Type::Local, 3}, {ValuePlace::Type::Caller, 1}}});
    EXPECT_THAT(opMult.getArgumentPlaces(), testing::ElementsAre(ValuePlace{ValuePlace::Type::Local, 3}, ValuePlace{ValuePlace::Type::Caller, 1}));
    Value<float> val1, val2;
    val1.set(8.5F);
    val2.set(13.2F);
    std::array<ValueBase*, 2> args{ &val1, &val2 };
    OperationContext opCont{args.data()};
    opMult.execute(opCont);
    EXPECT_EQ(opCont.nextStep, 1);
    EXPECT_NEAR(val1.get(), 112.2F, 1E-6F);
    EXPECT_EQ(val2.get(), 13.2F);
}
