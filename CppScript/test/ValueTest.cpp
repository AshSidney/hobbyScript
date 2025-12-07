#include <gmock/gmock.h>

#include <CppScript/Value.h>
#include "ValueMock.h"
#include "TestUtils.h"

using namespace CppScript;
using namespace CppScriptTest;

TEST(ValueTest, StdIntValueAndType)
{
    Value<int> valInt;
    EXPECT_EQ(valInt.getTypeId(), Value<int>::typeId);
    EXPECT_TRUE(valInt.getTypeId().isFinalType);
    EXPECT_FALSE(valInt.getTypeId().commonTypeId->isFinalType);
    EXPECT_EQ(valInt.getTypeId().commonTypeId, &ValueCommon<int>::typeId);
    EXPECT_EQ(valInt.getTypeId().getCommonPtrId(), &ValueCommonPtr<int>::typeId);
    EXPECT_EQ(valInt.getTypeId().layout.size, sizeof(valInt));
    EXPECT_EQ(valInt.getTypeId().layout.alignment, alignof(decltype(valInt)));
    EXPECT_FALSE(valInt.getTypeId().isReference());
    EXPECT_EQ(valInt.getTypeId().enumItems.size(), 0);

    EXPECT_THROW(valInt.get(), ValueNotAvailable);
    valInt.set(123);
    EXPECT_EQ(valInt.get(), 123);

    Value<const int> constInt;
    EXPECT_NE(valInt.getTypeId(), constInt.getTypeId());
    EXPECT_TRUE(constInt.getTypeId().isFinalType);
    EXPECT_EQ(constInt.getTypeId().commonTypeId, &ValueCommon<int>::typeId);
    EXPECT_EQ(constInt.getTypeId().getCommonPtrId(), &ValueCommonPtr<int>::typeId);
    EXPECT_EQ(constInt.getTypeId().layout.size, sizeof(constInt));
    EXPECT_EQ(constInt.getTypeId().layout.alignment, alignof(Value<const int>));
    EXPECT_FALSE(constInt.getTypeId().isReference());
    EXPECT_EQ(valInt.getTypeId().commonTypeId, constInt.getTypeId().commonTypeId);

    constInt.set(456);
    EXPECT_EQ(constInt.get(), 456);
}

TEST(ValueTest, FloatValuePointer)
{
    Value<float*> floatPtr;
    EXPECT_TRUE(floatPtr.getTypeId().isFinalType);
    EXPECT_FALSE(floatPtr.getTypeId().getCommonPtrId()->isFinalType);
    EXPECT_EQ(*floatPtr.getTypeId().getCommonPtrId(), ValueCommonPtr<float>::typeId);
    EXPECT_EQ(floatPtr.getTypeId().layout.size, sizeof(floatPtr));
    EXPECT_EQ(floatPtr.getTypeId().layout.alignment, alignof(Value<float*>));
    EXPECT_TRUE(floatPtr.getTypeId().isReference());

    EXPECT_EQ(floatPtr.get(), nullptr);
    float valFloat{ 567.89F };
    floatPtr.set(&valFloat);
    EXPECT_EQ(floatPtr.get(), &valFloat);

    Value<const float*> floatConstPtr;
    EXPECT_TRUE(floatConstPtr.getTypeId().isFinalType);
    EXPECT_FALSE(floatConstPtr.getTypeId().getCommonPtrId()->isFinalType);
    EXPECT_EQ(*floatConstPtr.getTypeId().getCommonPtrId(), ValueCommonPtr<float>::typeId);
    EXPECT_EQ(floatConstPtr.getTypeId().layout.size, sizeof(floatConstPtr));
    EXPECT_EQ(floatConstPtr.getTypeId().layout.alignment, alignof(decltype(floatConstPtr)));
    EXPECT_TRUE(floatConstPtr.getTypeId().isReference());
    EXPECT_NE(floatPtr.getTypeId(), floatConstPtr.getTypeId());
    EXPECT_EQ(floatPtr.getTypeId().commonTypeId, floatConstPtr.getTypeId().commonTypeId);

    EXPECT_EQ(floatConstPtr.get(), nullptr);
    float constFloat{ -1.876F };
    floatConstPtr.set(&constFloat);
    EXPECT_EQ(floatConstPtr.get(), &constFloat);
}

TEST(ValueTest, StructLReference)
{
    Value<TestStruct> structVal;

    Value<TestStruct&> structRef;
    EXPECT_TRUE(structRef.getTypeId().isFinalType);
    EXPECT_FALSE(structRef.getTypeId().getCommonPtrId()->isFinalType);
    EXPECT_EQ(*structRef.getTypeId().getCommonPtrId(), ValueCommonPtr<TestStruct>::typeId);
    EXPECT_EQ(structRef.getTypeId().layout.size, sizeof(structRef));
    EXPECT_EQ(structRef.getTypeId().layout.alignment, alignof(decltype(structRef)));
    EXPECT_EQ(structVal.getTypeId().layout.size, sizeof(structVal));
    EXPECT_EQ(structVal.getTypeId().layout.alignment, alignof(Value<TestStruct>));
    EXPECT_NE(structRef.getTypeId(), structVal.getTypeId());
    EXPECT_EQ(structRef.getTypeId().commonTypeId, structVal.getTypeId().commonTypeId->commonTypeId);

    EXPECT_THROW(structRef.get(), ValueNotAvailable);
    TestStruct testVal{ 321, true };
    structRef.set(testVal);
    EXPECT_EQ(&structRef.get(), &testVal);

    Value<const TestStruct&> structConstRef;
    EXPECT_TRUE(structConstRef.getTypeId().isFinalType);
    EXPECT_FALSE(structConstRef.getTypeId().getCommonPtrId()->isFinalType);
    EXPECT_EQ(*structConstRef.getTypeId().getCommonPtrId(), ValueCommonPtr<TestStruct>::typeId);
    EXPECT_EQ(structConstRef.getTypeId().layout.size, sizeof(structConstRef));
    EXPECT_EQ(structConstRef.getTypeId().layout.alignment, alignof(decltype(structConstRef)));
    EXPECT_NE(structRef.getTypeId(), structConstRef.getTypeId());
    EXPECT_EQ(structRef.getTypeId().commonTypeId, structConstRef.getTypeId().commonTypeId);

    EXPECT_THROW(structConstRef.get(), ValueNotAvailable);
    const TestStruct constVal{ -78, false };
    structConstRef.set(constVal);
    EXPECT_EQ(&structConstRef.get(), &constVal);
}

TEST(ValueTest, Array)
{
    Value<const int[3]> valArrInt;
    EXPECT_TRUE(valArrInt.getTypeId().isFinalType);
    EXPECT_FALSE(valArrInt.getTypeId().getCommonPtrId()->isFinalType);
    EXPECT_EQ(*valArrInt.getTypeId().getCommonPtrId(), ValueCommonPtr<int>::typeId);
    EXPECT_EQ(valArrInt.getTypeId().layout.size, sizeof(valArrInt));
    EXPECT_EQ(valArrInt.getTypeId().layout.alignment, alignof(decltype(valArrInt)));
    EXPECT_TRUE(valArrInt.getTypeId().isReference());
    
    const int intArr[3] = {4, 9, 16};
    valArrInt.set(intArr);
    EXPECT_EQ(valArrInt.get()[0], 4);
    EXPECT_EQ(valArrInt.get()[1], 9);
    EXPECT_EQ(valArrInt.get()[2], 16);

    Value<TestStruct[]> valArrStruc;
    EXPECT_TRUE(valArrStruc.getTypeId().isFinalType);
    EXPECT_FALSE(valArrStruc.getTypeId().getCommonPtrId()->isFinalType);
    EXPECT_EQ(*valArrStruc.getTypeId().getCommonPtrId(), ValueCommonPtr<TestStruct>::typeId);
    EXPECT_EQ(valArrStruc.getTypeId().layout.size, sizeof(valArrStruc));
    EXPECT_EQ(valArrStruc.getTypeId().layout.alignment, alignof(decltype(valArrStruc)));
    EXPECT_TRUE(valArrStruc.getTypeId().isReference());

    TestStruct strucArr[2]{ {8, true}, {7, false}};
    valArrStruc.set(strucArr);
    EXPECT_EQ(valArrStruc.get()[0].val, 8);
    EXPECT_TRUE(valArrStruc.get()[0].flag);
    EXPECT_EQ(valArrStruc.get()[1].val, 7);
    EXPECT_FALSE(valArrStruc.get()[1].flag);

    Value<float[3][3]> vallArrFloat;
    EXPECT_TRUE(vallArrFloat.getTypeId().isFinalType);
    EXPECT_FALSE(vallArrFloat.getTypeId().getCommonPtrId()->isFinalType);
    EXPECT_EQ(*vallArrFloat.getTypeId().getCommonPtrId(), ValueCommonPtr<float[3]>::typeId);
    EXPECT_EQ(vallArrFloat.getTypeId().layout.size, sizeof(vallArrFloat));
    EXPECT_EQ(vallArrFloat.getTypeId().layout.alignment, alignof(decltype(vallArrFloat)));
    EXPECT_TRUE(vallArrFloat.getTypeId().isReference());
    
    float floatMat[3][3]{1, 0, 0,  0, 1, -1,  0, 0, 2};
    vallArrFloat.set(floatMat);
    EXPECT_EQ(vallArrFloat.get()[0][0], 1);    
    EXPECT_EQ(vallArrFloat.get()[0][1], 0);    
    EXPECT_EQ(vallArrFloat.get()[0][2], 0);    
    EXPECT_EQ(vallArrFloat.get()[1][0], 0);    
    EXPECT_EQ(vallArrFloat.get()[1][1], 1);    
    EXPECT_EQ(vallArrFloat.get()[1][2], -1);    
    EXPECT_EQ(vallArrFloat.get()[2][0], 0);    
    EXPECT_EQ(vallArrFloat.get()[2][1], 0);    
    EXPECT_EQ(vallArrFloat.get()[2][2], 2);    
}

enum class TestValEnum{ Val1, OtherVal, Third };

namespace CppScript
{

template <>
constexpr auto enumItems<TestValEnum>()
{
    return std::make_tuple(Id{"TestValEnum"},
        EnumItem<TestValEnum, TestValEnum::OtherVal>{"val2"},
        EnumItem<TestValEnum, TestValEnum::Val1>{"val1"},
        EnumItem<TestValEnum, TestValEnum::Third>{"val3"});
}

}

TEST(ValueTest, Enum)
{
    const Value<TestValEnum> enumVal;
    EXPECT_TRUE(enumVal.getTypeId().isFinalType);
    EXPECT_EQ(*enumVal.getTypeId().commonTypeId, ValueCommon<TestValEnum>::typeId);
    EXPECT_EQ(enumVal.getTypeId().layout.size, sizeof(enumVal));
    EXPECT_EQ(enumVal.getTypeId().layout.alignment, alignof(decltype(enumVal)));
    EXPECT_FALSE(enumVal.getTypeId().isReference());
    EXPECT_THAT(span2Vector(enumVal.getTypeId().enumItems), testing::ElementsAre("val1", "val2", "val3"));
}

TEST(ValueTest, TypeIdCreateValue)
{
    char buffer[1024];
    void* buffPtr = buffer;
    auto* val = Value<int>::typeId.create(buffPtr);
    EXPECT_EQ(val, buffPtr);
    EXPECT_EQ(val->getTypeId(), Value<int>::typeId);

    auto& valInt = static_cast<Value<int>&>(*val);
    valInt.set(-753);
    EXPECT_EQ(valInt.get(), -753);

    buffPtr = static_cast<char*>(buffPtr) + sizeof(val->getTypeId().layout.size);
    auto* val2 = Value<float>::typeId.create(buffPtr);
    EXPECT_EQ(val2, buffPtr);
    EXPECT_EQ(val2->getTypeId(), Value<float>::typeId);

    auto& valFloat = static_cast<Value<float>&>(*val2);
    valFloat.set(12.34F);
    EXPECT_EQ(valFloat.get(), 12.34F);

    auto destructList = TestStructDestruct::initDestructList();
    buffPtr = static_cast<char*>(buffPtr) + sizeof(val2->getTypeId().layout.size);
    auto* val3 = Value<TestStructDestruct>::typeId.create(buffPtr);
    EXPECT_EQ(val3, buffPtr);
    EXPECT_EQ(val3->getTypeId(), Value<TestStructDestruct>::typeId);

    auto& valStruct = static_cast<Value<TestStructDestruct>&>(*val3);
    EXPECT_EQ(destructList->size(), 0);
    valStruct.set({ 369, true });
    EXPECT_EQ(destructList->size(), 1);
    EXPECT_EQ(valStruct.get().val, 369);
    EXPECT_TRUE(valStruct.get().flag);

    const auto* valStructPtr = &valStruct.get();
    val3->~ValueBase();
    EXPECT_EQ(destructList->size(), 2);
    EXPECT_EQ(valStructPtr, destructList->back());
}

TEST(ValueTest, TypeIdMatchesParameterType)
{
    EXPECT_EQ(Value<std::string>::typeId.matchesType(Value<std::string>::typeId), TypeId::ArgumentMatch::Exact);
    EXPECT_EQ(Value<int>::typeId.matchesType(Value<float>::typeId), TypeId::ArgumentMatch::Unrelated);
    EXPECT_EQ(Value<float>::typeId.matchesType(Value<const float>::typeId), TypeId::ArgumentMatch::Cast);
    EXPECT_EQ(Value<float>::typeId.matchesType(Value<const float&>::typeId), TypeId::ArgumentMatch::Cast);
    EXPECT_EQ(Value<float>::typeId.matchesType(Value<float*>::typeId), TypeId::ArgumentMatch::Cast);
    EXPECT_EQ(Value<const float>::typeId.matchesType(Value<const float>::typeId), TypeId::ArgumentMatch::Exact);
    EXPECT_EQ(Value<const float>::typeId.matchesType(Value<const float&>::typeId), TypeId::ArgumentMatch::Cast);
    EXPECT_EQ(Value<const float>::typeId.matchesType(Value<float*>::typeId), TypeId::ArgumentMatch::Cast);
    EXPECT_EQ(Value<float*>::typeId.matchesType(Value<float>::typeId), TypeId::ArgumentMatch::Cast);
    EXPECT_EQ(Value<float*>::typeId.matchesType(Value<const float*>::typeId), TypeId::ArgumentMatch::ConstMismatch);
    EXPECT_EQ(Value<float*>::typeId.matchesType(Value<float&>::typeId), TypeId::ArgumentMatch::Cast);
    EXPECT_EQ(Value<const float*>::typeId.matchesType(Value<float>::typeId), TypeId::ArgumentMatch::Cast);
    EXPECT_EQ(Value<const float*>::typeId.matchesType(Value<const float*>::typeId), TypeId::ArgumentMatch::Exact);
    EXPECT_EQ(Value<const float*>::typeId.matchesType(Value<float&>::typeId), TypeId::ArgumentMatch::Cast);
    EXPECT_EQ(Value<float&>::typeId.matchesType(Value<float>::typeId), TypeId::ArgumentMatch::Cast);
    EXPECT_EQ(Value<float&>::typeId.matchesType(Value<const float>::typeId), TypeId::ArgumentMatch::ConstMismatch);
    EXPECT_EQ(Value<float&>::typeId.matchesType(Value<const float&>::typeId), TypeId::ArgumentMatch::ConstMismatch);
    EXPECT_EQ(Value<float&>::typeId.matchesType(Value<float*>::typeId), TypeId::ArgumentMatch::Cast);
    EXPECT_EQ(Value<const float&>::typeId.matchesType(Value<float>::typeId), TypeId::ArgumentMatch::Cast);
    EXPECT_EQ(Value<const float&>::typeId.matchesType(Value<const float>::typeId), TypeId::ArgumentMatch::Cast);
    EXPECT_EQ(Value<const float&>::typeId.matchesType(Value<const float&>::typeId), TypeId::ArgumentMatch::Exact);
    EXPECT_EQ(Value<const float&>::typeId.matchesType(Value<float*>::typeId), TypeId::ArgumentMatch::Cast);
}

TEST(ValueTest, TypeIdAcceptsParameterType)
{
    EXPECT_TRUE(Value<std::string>::typeId.accepts(Value<std::string>::typeId));
    EXPECT_FALSE(Value<int>::typeId.accepts(Value<float>::typeId));
    EXPECT_TRUE(Value<float>::typeId.accepts(Value<const float>::typeId));
    EXPECT_TRUE(Value<float>::typeId.accepts(Value<const float&>::typeId));
    EXPECT_TRUE(Value<float>::typeId.accepts(Value<float*>::typeId));
    EXPECT_TRUE(Value<const float>::typeId.accepts(Value<const float>::typeId));
    EXPECT_TRUE(Value<const float>::typeId.accepts(Value<const float&>::typeId));
    EXPECT_TRUE(Value<const float>::typeId.accepts(Value<float*>::typeId));
    EXPECT_TRUE(Value<float*>::typeId.accepts(Value<float>::typeId));
    EXPECT_FALSE(Value<float*>::typeId.accepts(Value<const float*>::typeId));
    EXPECT_TRUE(Value<float*>::typeId.accepts(Value<float&>::typeId));
    EXPECT_TRUE(Value<const float*>::typeId.accepts(Value<float>::typeId));
    EXPECT_TRUE(Value<const float*>::typeId.accepts(Value<const float*>::typeId));
    EXPECT_TRUE(Value<const float*>::typeId.accepts(Value<float&>::typeId));
    EXPECT_TRUE(Value<float&>::typeId.accepts(Value<float>::typeId));
    EXPECT_FALSE(Value<float&>::typeId.accepts(Value<const float>::typeId));
    EXPECT_FALSE(Value<float&>::typeId.accepts(Value<const float&>::typeId));
    EXPECT_TRUE(Value<float&>::typeId.accepts(Value<float*>::typeId));
    EXPECT_TRUE(Value<const float&>::typeId.accepts(Value<float>::typeId));
    EXPECT_TRUE(Value<const float&>::typeId.accepts(Value<const float>::typeId));
    EXPECT_TRUE(Value<const float&>::typeId.accepts(Value<const float&>::typeId));
    EXPECT_TRUE(Value<const float&>::typeId.accepts(Value<float*>::typeId));
}