#include<gtest/gtest.h>

#include <CppScript/Types.h>
#include <CppScript/BasicTypes.h>

using namespace CppScript;


TEST(TypesTest, BaseTypesIds)
{
	EXPECT_EQ(TypeInt::id().getName(), "int");
	EXPECT_EQ(TypeFloat::id().getName(), "float");
	EXPECT_EQ(TypeBool::id().getName(), "bool");
}

TEST(TypesTest, BaseTypesCasts)
{
	TypeBase::Ref intVal = TypeInt::create(15);
	TypeBase::Ref floatVal = TypeFloat::create(47.58);
	TypeBase::Ref boolVal = TypeBool::create(true);

	EXPECT_EQ(TypeInt::id().get(*intVal), 15);
	EXPECT_THROW(TypeInt::id().get(*floatVal), InvalidTypeCast);
	EXPECT_EQ(TypeFloat::id().get(*floatVal), 47.58);
	EXPECT_THROW(TypeFloat::id().get(*intVal), InvalidTypeCast);
	EXPECT_EQ(TypeBool::id().get(*boolVal), true);
	EXPECT_THROW(TypeBool::id().get(*floatVal), InvalidTypeCast);

	EXPECT_EQ(intVal->as<long long>(), 15);
	EXPECT_EQ(intVal->as<short>(), 15);
	EXPECT_EQ(floatVal->as<double>(), 47.58);
	EXPECT_EQ(floatVal->as<float>(), 47.58f);
}

TEST(TypesTest, BaseTypesCastOverflow)
{
	TypeBase::Ref largeIntVal = TypeInt::create(0x789456ab);
	TypeBase::Ref largeFloatVal = TypeFloat::create(1.234e40);

	EXPECT_EQ(largeIntVal->as<int>(), 0x789456ab);
	using ShortValueOverflow = ValueOverflow<short, TypeInt::ValueType>;
	EXPECT_THROW(largeIntVal->as<short>(), ShortValueOverflow);

	EXPECT_EQ(largeFloatVal->as<double>(), 1.234e40);
	using FloatValueOverflow = ValueOverflow<float, TypeFloat::ValueType>;
	EXPECT_THROW(largeFloatVal->as<float>(), FloatValueOverflow);
}


TEST(TypesTest, AddOperator)
{
	auto intVal = TypeInt::create(125);
	auto intVal2 = TypeInt::create(3459);

	(*intVal) += *intVal2;
	EXPECT_EQ(intVal->get(), 3584);
	EXPECT_EQ(intVal2->get(), 3459);

	(*intVal2) += TypeInt{ 753 };
	EXPECT_EQ(intVal2->get(), 4212);
}

TEST(TypesTest, AddOperatorCastIntToFloat)
{
	auto floatVal = TypeFloat::create(125.7);
	auto intVal = TypeInt::create(345);

	(*floatVal) += *intVal;
	EXPECT_EQ(floatVal->get(), 470.7);
	EXPECT_EQ(intVal->get(), 345);

	EXPECT_THROW((*intVal) += *floatVal, InvalidTypeCast);
}

TEST(TypesTest, CloneBool)
{
	auto trueVal = TypeBool::trueValue;
	auto falseVal = TypeBool::falseValue;
	EXPECT_TRUE(trueVal->as<BoolValue>());
	EXPECT_FALSE(falseVal->as<BoolValue>());
	EXPECT_EQ(trueVal, TypeBool::trueValue);
	EXPECT_EQ(falseVal, TypeBool::falseValue);

	auto otherTrueVal = trueVal->clone();
	EXPECT_EQ(otherTrueVal, TypeBool::trueValue);
	auto otherFalseVal = falseVal->clone();
	EXPECT_EQ(otherFalseVal, TypeBool::falseValue);
}