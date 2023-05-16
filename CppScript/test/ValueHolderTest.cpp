#include <gtest/gtest.h>

#include <CppScript/ValueHolder.h>
#include <CppScript/IntValue.h>

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
 
    EXPECT_EQ(intVal.getTypeId(), intVal2.getTypeId());
    EXPECT_EQ(intVal.getTypeId(), intVal3.getTypeId());
    EXPECT_NE(intVal.getTypeId(), floatVal.getTypeId());

    EXPECT_NE(intVal.getSpecTypeId(), intVal2.getSpecTypeId());
    EXPECT_EQ(intVal.getSpecTypeId(), intVal3.getSpecTypeId());
}

TEST(ValueHolderTest, ValueTraits)
{
    SpecTypeValueHolder<IntValue> intVal;
    intVal.set(12345678_I);

    EXPECT_EQ(ValueTraits<const IntValue&>::get(intVal), 12345678_I);

    ValueTraits<const IntValue>::set(intVal, -98765_I);
    EXPECT_EQ(ValueTraits<const IntValue&>::get(intVal), -98765_I);
}