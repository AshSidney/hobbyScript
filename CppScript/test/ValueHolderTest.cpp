#include <gtest/gtest.h>

#include <CppScript/ValueHolder.h>
#include <CppScript/IntValue.h>

#include "TestUtils.h"

using namespace CppScript;

TEST(ValueHolderTest, ValueAndType)
{
    SpecTypeValueHolder<IntValue> intVal;
    intVal.set(1234_I);
    SpecTypeValueHolder<const IntValue> intVal2;
    intVal2.set(567_I);
    
    EXPECT_EQ(intVal.get(), 1234_I);
    EXPECT_EQ(intVal2.get(), 567_I);
 
    SpecTypeValueHolder<IntValue> intVal3;

    SpecTypeValueHolder<float> floatVal;
 
    EXPECT_EQ(intVal.getTypeId().basicTypeId, intVal2.getTypeId().basicTypeId);
    EXPECT_EQ(intVal.getTypeId().basicTypeId, intVal3.getTypeId().basicTypeId);
    EXPECT_NE(intVal.getTypeId().basicTypeId, floatVal.getTypeId().basicTypeId);

    EXPECT_NE(intVal.getTypeId(), intVal2.getTypeId());
    EXPECT_EQ(intVal.getTypeId(), intVal3.getTypeId());
}

TEST(ValueHolderTest, TypeId_ConstructValueHolder)
{
    const ValueTypeId<IntValue>& intTypeId{ SpecTypeValueHolder<IntValue>::specTypeId };
    const ValueTypeId<float>& floatTypeId{ SpecTypeValueHolder<float>::specTypeId };
    const ValueTypeId<bool>& boolTypeId{ SpecTypeValueHolder<bool>::specTypeId };
    const ValueTypeId<const IntValue&>& intTypeRefId{ SpecTypeValueHolder<const IntValue&>::specTypeId };
    EXPECT_EQ(intTypeId.layout, makeTypeLayout<SpecTypeValueHolder<IntValue>>());
    EXPECT_EQ(floatTypeId.layout, makeTypeLayout<SpecTypeValueHolder<float>>());
    EXPECT_EQ(boolTypeId.layout, makeTypeLayout<SpecTypeValueHolder<bool>>());
    EXPECT_EQ(intTypeRefId.layout, makeTypeLayout<SpecTypeValueHolder<const IntValue&>>());
    auto buffer = std::make_unique<char[]>(intTypeId.layout.size + floatTypeId.layout.size + boolTypeId.layout.size + intTypeRefId.layout.size);
    auto* intVal = static_cast<SpecTypeValueHolder<IntValue>*>(intTypeId.construct(buffer.get()));
    auto* floatVal = static_cast<SpecTypeValueHolder<float>*>(floatTypeId.construct(buffer.get() + intTypeId.layout.size));
    auto* boolVal = static_cast<SpecTypeValueHolder<bool>*>(boolTypeId.construct(buffer.get() + intTypeId.layout.size + floatTypeId.layout.size));
    auto* intRefVal = static_cast<SpecTypeValueHolder<const IntValue&>*>(intTypeRefId.construct(buffer.get()
        + intTypeId.layout.size + floatTypeId.layout.size + boolTypeId.layout.size));
    intVal->set(789456_I);
    floatVal->set(123.456F);
    boolVal->set(true);
    intRefVal->set(intVal->get());
    EXPECT_EQ(intVal->get(), 789456_I);
    EXPECT_EQ(floatVal->get(), 123.456F);
    EXPECT_TRUE(boolVal->get());
    EXPECT_EQ(intRefVal->get(), 789456_I);
}

TEST(ValueHolderTest, ValueHolder_ConstructRef)
{
    const ValueTypeId<IntValue>& intTypeId{ SpecTypeValueHolder<IntValue>::specTypeId };
    auto buffer = std::make_unique<char[]>(intTypeId.layout.size * 2);
    auto* intVal = static_cast<SpecTypeValueHolder<IntValue>*>(intTypeId.construct(buffer.get()));
    intVal->set(3333444455556666_I);
    auto* intValRef = static_cast<SpecTypeValueHolder<const IntValue&>*>(intVal->constructRef(buffer.get() + intTypeId.layout.size));
    EXPECT_EQ(intVal->get(), 3333444455556666_I);
    EXPECT_EQ(intValRef->get(), 3333444455556666_I);
}

TEST(ValueHolderTest, ValueTraits)
{
    SpecTypeValueHolder<IntValue> intVal;
    intVal.set(12345678_I);

    EXPECT_EQ(ValueTraits<const IntValue&>::get(intVal), 12345678_I);

    ValueTraits<const IntValue>::set(intVal, -98765_I);
    EXPECT_EQ(ValueTraits<const IntValue&>::get(intVal), -98765_I);
}