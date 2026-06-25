#include <gmock/gmock.h>

#include <CppScript/Operation.h>
#include <CppScript/IntValue.h>
#include "OperationResolverProxy.h"
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


using TestOperationBuilder = OperationBuilder<TestOpAdd, TestOpAddFloat, TestOpSetVec, TestOpVecNorm, TestOpFloatCompare>;

using TestOperationBuilder1 = OperationBuilder<TestOpAdd, TestOpAddFloat>;

using TestOperationBuilderExtended = OperationBuilders<TestOperationBuilder1, OperationBuilder<TestOpSetVec, TestOpVecNorm, TestOpFloatCompare>>;

using TestOperationBuilderExtended2 = OperationBuilders<TestOperationBuilder1, OperationBuilder<TestOpSetVec>, OperationBuilder<TestOpVecNorm, TestOpFloatCompare>>;

static_assert(std::is_same_v<TestOperationBuilder, TestOperationBuilderExtended>);
static_assert(std::is_same_v<TestOperationBuilder, TestOperationBuilderExtended2>);

using TestOperationBuilder0 = OperationBuilder<TestOpAdd>;

static_assert(std::is_same_v<TestOperationBuilder1, OperationBuilders<TestOperationBuilder0, OperationBuilder<TestOpAddFloat>>>);

static_assert(std::is_same_v<OperationBuilders<TestOperationBuilder0, OperationBuilder<TestOpAddFloat, TestOpFloatCompare>>,
    OperationBuilders<TestOperationBuilder1, OperationBuilder<TestOpFloatCompare>>>);


template <typename OB>
typename OB::OperationType resolveValidOperation(const OB& builder, OperationResolutionData opData, TypeFrames& frames, std::size_t expectedOpIndex)
{
    OperationResolverProxy resolver(builder.getResolver());
    OperationResolverProxy::ResolutionContext context{ frames };
    EXPECT_TRUE(resolver.resolveOperation(opData, context));
    EXPECT_EQ(context.blockContext.operations.size(), 1);
    auto resOp = context.blockContext.operations[0];
    EXPECT_EQ(resOp.index, expectedOpIndex);
    std::vector<ValuePlace> checkArgs{ opData.argumentPlaces };
    if (opData.returnPlace)
        checkArgs.push_back(*opData.returnPlace);
    EXPECT_EQ(resOp.argumentPlaces, checkArgs);
    EXPECT_EQ(resOp.jumps, opData.jumps);
    EXPECT_EQ(resOp.location, opData.location);

    auto operation = builder.build(std::move(resOp));
    EXPECT_EQ(operation.getArgumentPlaces(), checkArgs);
    EXPECT_EQ(operation.getLocation(), opData.location);
    return operation; 
}

TestOperationBuilder::OperationType resolveValidOperation(OperationResolutionData opData, TypeFrames& frames, std::size_t expectedOpIndex)
{
    TestOperationBuilder builder;
    return resolveValidOperation(builder, std::move(opData), frames, expectedOpIndex);
}


TEST(OperationTest, IntAdd)
{
    Value<int> valInt1, valInt2;
    valInt1.set(45);
    valInt2.set(678);

    TypeFrames frames;
    auto& localFrame = getFrame(frames, ValuePlace::Type::Local);
    localFrame = {&valInt1.getTypeId(), &valInt2.getTypeId()};
    auto opAdd = resolveValidOperation({ {"+="}, {}, { {ValuePlace::Type::Local, 0}, {ValuePlace::Type::Local, 1} }, {1}, {10,5} },
        frames, 0);
    
    std::array<ValueBase*, 2> args1{ &valInt1, &valInt2 };
    OperationContext opCont1{args1.data()};
    opAdd.execute(opCont1);
    EXPECT_EQ(opCont1.nextStep, 1);
    EXPECT_EQ(valInt1.get(), 723);
    EXPECT_EQ(valInt2.get(), 678);

    const auto opAddRet = resolveValidOperation({ {"+="}, ValuePlace{ValuePlace::Type::Local, 2}, { {ValuePlace::Type::Local, 1}, {ValuePlace::Type::Local, 0} }, {1}, {12,3} },
        frames, 0);
    EXPECT_EQ(localFrame.size(), 3);
    EXPECT_EQ(*localFrame[2], Value<int&>::typeId);

    Value<int&> refInt;
    std::array<ValueBase*, 3> args2{ &valInt2, &valInt1, &refInt };
    OperationContext opCont2{args2.data()};
    opAddRet.execute(opCont2);
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
    
    TypeFrames frames;
    getFrame(frames, ValuePlace::Type::Local) = {nullptr, nullptr, &valFloat2.getTypeId()};
    getFrame(frames, ValuePlace::Type::Module) = {nullptr, &valFloat1.getTypeId()};
    auto opAdd = resolveValidOperation({ {"+="}, ValuePlace{ValuePlace::Type::Caller, 3},
        { {ValuePlace::Type::Module, 1}, {ValuePlace::Type::Local, 2} }, {-5}, {148, 22} }, frames, 1);
    EXPECT_THAT(getFrame(frames, ValuePlace::Type::Caller), testing::ElementsAre(nullptr, nullptr, nullptr, &valFloat3.getTypeId()));

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

    TypeFrames frames;
    getFrame(frames, ValuePlace::Type::Local) = {nullptr, &vec.getTypeId()};
    getFrame(frames, ValuePlace::Type::Module) = {nullptr, &vecVal.getTypeId()};
    auto setVecOp = resolveValidOperation({ {"setVec", "testVec"}, {}, {{ValuePlace::Type::Local, 1}, {ValuePlace::Type::Module, 1}},
        {3}, {234, 53} }, frames, 2);

    std::array<ValueBase*, 2> args{&vec, &vecVal};
    OperationContext opCont{args.data()};
    setVecOp.execute(opCont);
    EXPECT_EQ(opCont.nextStep, 3);
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

    TypeFrames frames;
    getFrame(frames, ValuePlace::Type::Module) = {nullptr, nullptr, &vec.getTypeId()};
    auto normVecOp = resolveValidOperation({ {"norm", "testVec"}, {}, {{ValuePlace::Type::Module, 2}},
        {1}, {78, 9} }, frames, 3);

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

    TypeFrames frames;
    getFrame(frames, ValuePlace::Type::Local) = {nullptr, &valFloat2.getTypeId()};
    getFrame(frames, ValuePlace::Type::Module) = {nullptr, nullptr, &valFloat1.getTypeId()};
    auto opCompare = resolveValidOperation({ {"<=>"}, {}, {{ValuePlace::Type::Module, 2}, {ValuePlace::Type::Local, 1}},
        {-2, 7, 3}, {45, 16} }, frames, 4);
    
    std::array<ValueBase*, 2> args{ &valFloat1, &valFloat2 };
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
    const TestOperationBuilder builder;
    const std::vector<TestOperationBuilder::OperationType> operations{ builder.build(std::vector<OperationBuildContext>
        {{0, {{ValuePlace::Type::Local, 0}, {ValuePlace::Type::Module, 1}, {ValuePlace::Type::Local, 2}}, {1}},
        {1, {{ValuePlace::Type::Module, 3}, {ValuePlace::Type::Local, 4}}, {-1}},
        {4, {{ValuePlace::Type::Local, 8}, {ValuePlace::Type::Local, 2}}, {2, -3, 5}},
        {2, {{ValuePlace::Type::Module, 5}, {ValuePlace::Type::Module, 6}}},
        {4, {{ValuePlace::Type::Local, 1}, {ValuePlace::Type::Module, 0}}, {1, 3, -2}},
        {3, {{ValuePlace::Type::Module, 3}}} }) };
    const auto argValues = getArgumentPlaces(operations);
    EXPECT_EQ(argValues.size(), 6);
    EXPECT_THAT(*argValues[0], testing::ElementsAre(ValuePlace{ValuePlace::Type::Local, 0},
        ValuePlace{ValuePlace::Type::Module, 1}, ValuePlace{ValuePlace::Type::Local, 2}));
    EXPECT_THAT(*argValues[1], testing::ElementsAre(ValuePlace{ValuePlace::Type::Module, 3}, ValuePlace{ValuePlace::Type::Local, 4}));
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
        if (id != Id{})
            EXPECT_CALL(*this, initialize(testing::_)).Times(1);
    }

    ~CustomOperationMock()
    {
        if (auto mockIt = findMock(*this); mockIt != mocks.end())
            mocks.erase(mockIt);
    }

    CustomOperationMock(CustomOperationMock&& other) noexcept
    {
        if (auto mockIt = other.findMock(other); mockIt != mocks.end())
            mockIt->mock = this;
    }

	MOCK_METHOD(void, initialize, (OperationBuildContext& context), ());
	MOCK_METHOD(void, executeVoid, (OperationContext& context), (const));
	MOCK_METHOD(void, executeRet, (OperationContext& context), (const));
	MOCK_METHOD(std::size_t, executeJump, (OperationContext& context), (const));

	MOCK_METHOD(void, execute, (OperationContext& context), (const, override));

    struct MockData
    {
        CustomOperationMock* mock;
        Id id;
    };

    static std::vector<CustomOperationMock*> getMocks(Id id)
    {
        std::vector<CustomOperationMock*> result;
        for (const MockData& mock : mocks)
        {
            if (mock.id == id)
                result.push_back(mock.mock);
        }
        return result;
    }

    std::vector<MockData>::iterator findMock(CustomOperationMock& mock)
    {
        return std::find_if(mocks.begin(), mocks.end(), [&mock](const MockData& mockData){ return &mock == mockData.mock; });
    }

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
    static constexpr bool voidReturn{ true };
	static constexpr std::size_t argumentCount{ 1 };
    static constexpr std::size_t jumpSize{ 1 };
    static constexpr Id id{ "firstOp" };

    CustomOperationMock1() : CustomOperationMock(id){}

    static OperationDescription getDescription()
    {
        static std::vector<const TypeId*> argTypes{ &Value<const float>::typeId, &Value<TestVec*>::typeId };
        return { id, nullptr, { argTypes.begin(), argTypes.end() }, 1 };
    }
};

class CustomOperationMock2 : public CustomOperationMock
{
public:
    static constexpr bool voidReturn{ false };
	static constexpr std::size_t argumentCount{ 2 };
    static constexpr std::size_t jumpSize{ 2 };
    static constexpr Id id{ "secOp", "testModule" };

    CustomOperationMock2() : CustomOperationMock(id){}

    static OperationDescription getDescription()
    {
        static std::vector<const TypeId*> argTypes{ &Value<int&>::typeId };
        return { id, &Value<bool>::typeId, { argTypes.begin(), argTypes.end() }, 2 };
    }
};

TEST(OperationTest, CustomOperationsAddMockClasses)
{
    TestOperationBuilder0 builder;
    const auto& resolver = builder.getResolver();
    ASSERT_EQ(resolver.getDescriptions().size(), 1);
    builder.addCustomOperations<CustomOperationMock1, CustomOperationMock2>();
    const auto& descrs = resolver.getDescriptions();
    ASSERT_EQ(descrs.size(), 3);
    EXPECT_EQ(descrs[0].id, Id{"+="});
    EXPECT_EQ(*descrs[0].returnType, Value<int&>::typeId);
    EXPECT_THAT(span2Vector(descrs[0].argumentTypes),
        testing::ElementsAre(&Value<int&>::typeId, &Value<const int>::typeId));
    EXPECT_EQ(descrs[0].jumpCount, 1);
    const auto descrMock1 = CustomOperationMock1::getDescription();
    EXPECT_EQ(descrs[1].id, descrMock1.id);
    EXPECT_EQ(descrs[1].returnType, descrMock1.returnType);
    EXPECT_THAT(descrs[1].argumentTypes, descrMock1.argumentTypes);
    EXPECT_EQ(descrs[1].jumpCount, descrMock1.jumpCount);
    const auto descrMock2 = CustomOperationMock2::getDescription();
    EXPECT_EQ(descrs[2].id, descrMock2.id);
    EXPECT_EQ(descrs[2].returnType, descrMock2.returnType);
    EXPECT_THAT(descrs[2].argumentTypes, descrMock2.argumentTypes);
    EXPECT_EQ(descrs[2].jumpCount, descrMock2.jumpCount);

    Value<int> valInt1, valInt2;
    valInt1.set(123);
    valInt2.set(789);
    TypeFrames frames;
    getFrame(frames, ValuePlace::Type::Local) = {&valInt1.getTypeId(), &valInt2.getTypeId()};
    auto opAdd = resolveValidOperation(builder, { {"+="}, {}, { {ValuePlace::Type::Local, 1}, {ValuePlace::Type::Local, 0} }, {1}, {120, 25} },
        frames, 0);
    std::array<ValueBase*, 2> args0{ &valInt1, &valInt2 };
    OperationContext opCont1{args0.data()};
    opAdd.execute(opCont1);
    EXPECT_EQ(opCont1.nextStep, 1);
    EXPECT_EQ(valInt1.get(), 912);
    EXPECT_EQ(valInt2.get(), 789);

    getFrame(frames, ValuePlace::Type::Local).push_back(&Value<float>::typeId);
    getFrame(frames, ValuePlace::Type::Caller) = {nullptr, &Value<TestVec>::typeId};
    auto opMock1 = resolveValidOperation(builder, { {"firstOp"}, {}, { {ValuePlace::Type::Local, 2}, {ValuePlace::Type::Caller, 1} },
        {4}, {178, 6} }, frames, 1);

    auto opMock2 = resolveValidOperation(builder, { {"secOp", "testModule"}, {}, { {ValuePlace::Type::Local, 0} },
        {-5, -2}, {179, 41} }, frames, 2);

    OperationContext context;
    auto* mock1 = CustomOperationMock::getMocks({"firstOp"}).front();
    EXPECT_CALL(*mock1, executeVoid(testing::Ref(context))).Times(1);
    opMock1.execute(context);
    EXPECT_EQ(context.nextStep, 4);

    auto* mock2 = CustomOperationMock::getMocks({"secOp", "testModule"}).front();
    EXPECT_CALL(*mock2, executeJump(testing::Ref(context)))
        .WillOnce(testing::Return(1));
    opMock2.execute(context);
    EXPECT_EQ(context.nextStep, -2);
    EXPECT_CALL(*mock2, executeJump(testing::Ref(context)))
        .WillOnce(testing::Return(0));
    opMock2.execute(context);
    EXPECT_EQ(context.nextStep, -5);
}

TEST(OperationTest, CustomOperationsAddTestClass)
{
    TestOperationBuilder1 builder;
    const auto& resolver = builder.getResolver();
    ASSERT_EQ(resolver.getDescriptions().size(), 2);
    builder.addCustomOperations<TestOpFloatMult>();
    ASSERT_EQ(resolver.getDescriptions().size(), 3);

    Value<float> val1, val2;
    val1.set(8.5F);
    val2.set(13.2F);
    TypeFrames frames;
    getFrame(frames, ValuePlace::Type::Local) = {&val1.getTypeId(), &val2.getTypeId()};
    auto opMult = resolveValidOperation(builder, { {"*="}, {}, { {ValuePlace::Type::Local, 0}, {ValuePlace::Type::Local, 1} }, {1}, {14, 10} },
        frames, 2);

    std::array<ValueBase*, 2> args{ &val1, &val2 };
    OperationContext opCont{args.data()};
    opMult.execute(opCont);
    EXPECT_EQ(opCont.nextStep, 1);
    EXPECT_NEAR(val1.get(), 112.2F, 1E-6F);
    EXPECT_EQ(val2.get(), 13.2F);
}
