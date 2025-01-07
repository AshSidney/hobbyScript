#include <gtest/gtest.h>

#include <CppScript/ValueHolder.h>
#include <CppScript/IntValue.h>

#include "TestUtils.h"

using namespace CppScript;

TEST(ValueHolderTest, ValueAndType)
{
    SpecTypeValueHolder<IntValue> intVal;
    intVal.setVal(1234_I);
    SpecTypeValueHolder<const IntValue> intVal2;
    intVal2.setVal(567_I);
    
    EXPECT_EQ(intVal.get(), 1234_I);
    EXPECT_EQ(intVal2.get(), 567_I);
 
    SpecTypeValueHolder<IntValue&> intValRef;
    intValRef.setVal(intVal.get());
    EXPECT_EQ(intValRef.get(), 1234_I);

    SpecTypeValueHolder<IntValue> intVal3;

    SpecTypeValueHolder<float> floatVal;

    EXPECT_EQ(intVal.getTypeId(), *intVal.getSpecTypeId().basicTypeId);
    EXPECT_EQ(floatVal.getTypeId(), *floatVal.getSpecTypeId().basicTypeId);
 
    EXPECT_EQ(intVal.getTypeId(), intVal2.getTypeId());
    EXPECT_EQ(intVal.getTypeId(), intValRef.getTypeId());
    EXPECT_EQ(intVal.getTypeId(), intVal3.getTypeId());
    EXPECT_NE(intVal.getTypeId(), floatVal.getTypeId());

    EXPECT_NE(intVal.getSpecTypeId(), intVal2.getSpecTypeId());
    EXPECT_NE(intVal.getSpecTypeId(), intValRef.getSpecTypeId());
    EXPECT_EQ(intVal.getSpecTypeId(), intVal3.getSpecTypeId());
    EXPECT_NE(intVal.getSpecTypeId(), floatVal.getSpecTypeId());

    EXPECT_FALSE(intVal.getSpecTypeId().isReference);
    EXPECT_FALSE(intVal2.getSpecTypeId().isReference);
    EXPECT_TRUE(intValRef.getSpecTypeId().isReference);
    EXPECT_FALSE(floatVal.getSpecTypeId().isReference);

    EXPECT_EQ(*intVal.getSpecTypeId().refTypeId, intValRef.getSpecTypeId());
    EXPECT_EQ(*intValRef.getSpecTypeId().refTypeId, intValRef.getSpecTypeId());
}

TEST(ValueHolderTest, TypeId_ConstructValueHolder)
{
    const ValueTypeId<IntValue>& intTypeId{ SpecTypeValueHolder<IntValue>::specTypeId };
    const ValueTypeId<float>& floatTypeId{ SpecTypeValueHolder<float>::specTypeId };
    const ValueTypeId<bool>& boolTypeId{ SpecTypeValueHolder<bool>::specTypeId };
    const ValueTypeId<const IntValue&>& intTypeRefId{ SpecTypeValueHolder<const IntValue&>::specTypeId };
    EXPECT_EQ(intTypeId.layout, TypeLayout::make<SpecTypeValueHolder<IntValue>>());
    EXPECT_EQ(floatTypeId.layout, TypeLayout::make<SpecTypeValueHolder<float>>());
    EXPECT_EQ(boolTypeId.layout, TypeLayout::make<SpecTypeValueHolder<bool>>());
    EXPECT_EQ(intTypeRefId.layout, TypeLayout::make<SpecTypeValueHolder<const IntValue&>>());
    auto buffer = std::make_unique<char[]>(intTypeId.layout.size + floatTypeId.layout.size + boolTypeId.layout.size + intTypeRefId.layout.size);
    auto* intVal = static_cast<SpecTypeValueHolder<IntValue>*>(intTypeId.construct(buffer.get()));
    auto* floatVal = static_cast<SpecTypeValueHolder<float>*>(floatTypeId.construct(buffer.get() + intTypeId.layout.size));
    auto* boolVal = static_cast<SpecTypeValueHolder<bool>*>(boolTypeId.construct(buffer.get() + intTypeId.layout.size + floatTypeId.layout.size));
    auto* intRefVal = static_cast<SpecTypeValueHolder<const IntValue&>*>(intTypeRefId.construct(buffer.get()
        + intTypeId.layout.size + floatTypeId.layout.size + boolTypeId.layout.size));
    intVal->setVal(789456_I);
    floatVal->setVal(123.456F);
    boolVal->setVal(true);
    intRefVal->setVal(intVal->get());
    EXPECT_EQ(intVal->get(), 789456_I);
    EXPECT_EQ(floatVal->get(), 123.456F);
    EXPECT_TRUE(boolVal->get());
    EXPECT_EQ(intRefVal->get(), 789456_I);
}

TEST(ValueHolderTest, TypeId_ConstructRef)
{
    SpecTypeValueHolder<IntValue> intVal;
    intVal.setVal(3333444455556666_I);
    auto buffer = std::make_unique<char[]>(intVal.specTypeId.layout.size);
    const ValueTypeId<const IntValue&>& intTypeRefId{ SpecTypeValueHolder<const IntValue&>::specTypeId };
    auto* intValRef = static_cast<SpecTypeValueHolder<const IntValue&>*>(intTypeRefId.constructRef(buffer.get(), intVal));
    ASSERT_NE(intValRef, nullptr);
    EXPECT_EQ(intVal.get(), 3333444455556666_I);
    EXPECT_EQ(intValRef->get(), 3333444455556666_I);
}

/*TEST(ValueHolderTest, ValueHolder_ConstructRef_Old)
{
    const ValueTypeId<IntValue>& intTypeId{ SpecTypeValueHolder<IntValue>::specTypeId };
    auto buffer = std::make_unique<char[]>(intTypeId.layout.size * 2);
    auto* intVal = static_cast<SpecTypeValueHolder<IntValue>*>(intTypeId.construct(buffer.get()));
    intVal->setVal(3333444455556666_I);
    auto* intValRef = static_cast<SpecTypeValueHolder<const IntValue&>*>(intVal->constructRef(buffer.get() + intTypeId.layout.size));
    EXPECT_EQ(intVal->get(), 3333444455556666_I);
    EXPECT_EQ(intValRef->get(), 3333444455556666_I);
}*/

TEST(ValueHolderTest, ValueTraits)
{
    SpecTypeValueHolder<IntValue> intVal;
    intVal.setVal(12345678_I);

    EXPECT_EQ(ValueTraits<const IntValue&>::get(intVal), 12345678_I);

    ValueTraits<const IntValue>::set(intVal, -98765_I);
    EXPECT_EQ(ValueTraits<const IntValue&>::get(intVal), -98765_I);
}