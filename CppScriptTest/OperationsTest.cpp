#include<gtest/gtest.h>

#include <CppScript/Operations.h>
#include <CppScript/Serializer.h>
#include <CppScript/Execution.h>
#include <CppScript/BasicTypes.h>

using namespace CppScript;

class OperationsFixture : public testing::Test
{
protected:
	Operation::Ref loadOperation(const Json& opData)
	{
		JsonLoader data{ opData };
		Operation::Ref operation;
		data.serialize(operation);
		return operation;
	}

	void setTestVariables(double var1, int var2)
	{
		executor.getContext().set("varOne", TypeFloat::create(var1));
		executor.getContext().set("varTwo", TypeInt::create(var2));
	}


	Context context;
	Executor executor{ context };
};
TEST_F(OperationsFixture, GetValue)
{
	auto valueProvider = loadOperation(R"( { "type" : "Value", "data" : 456 } )"_json);
	ASSERT_TRUE(bool(valueProvider));
	auto value = valueProvider->execute(executor);
	EXPECT_EQ(value->as<TypeInt::ValueType>(), 456);
}

TEST_F(OperationsFixture, GetValueIsNotClone)
{
	auto valueProvider = loadOperation(R"( { "type" : "Value", "data" : 789.12 } )"_json);
	ASSERT_TRUE(bool(valueProvider));
	auto value = valueProvider->execute(executor);
	EXPECT_EQ(value->as<TypeFloat::ValueType>(), 789.12);

	(*value) += *TypeFloat::create(15.6);
	auto valueChanged = valueProvider->execute(executor);
	EXPECT_EQ(valueChanged->as<TypeFloat::ValueType>(), 804.72);
}

TEST_F(OperationsFixture, ReadValue)
{
	auto valueReader = loadOperation(R"( { "type" : "Read", "data" : "varOne" } )"_json);
	ASSERT_TRUE(bool(valueReader));
	setTestVariables(9.625, -14);
	auto value = valueReader->execute(executor);
	EXPECT_EQ(value->as<TypeFloat::ValueType>(), 9.625);

	(*value) += *loadOperation(R"( { "type" : "Read", "data" : "varTwo" } )"_json)->execute(executor);
	auto valueChanged = valueReader->execute(executor);
	EXPECT_EQ(valueChanged->as<TypeFloat::ValueType>(), -4.375);
}

TEST_F(OperationsFixture, AssignValue)
{
	auto valueAssigner = loadOperation(R"( { "type" : "Assign", "data" : [ "varDest", { "type" : "Value", "data" : -159 } ] } )"_json);
	ASSERT_TRUE(bool(valueAssigner));
	auto value = valueAssigner->execute(executor);
	EXPECT_EQ(value->as<TypeInt::ValueType>(), -159);

	EXPECT_EQ(executor.getContext().get("varDest")->as<TypeInt::ValueType>(), -159);
}

TEST_F(OperationsFixture, CloneValue)
{
	auto valueProvider = loadOperation(R"( { "type" : "Clone", "data" : { "type" : "Value", "data" : 7410 } } )"_json);
	ASSERT_TRUE(bool(valueProvider));
	auto value = valueProvider->execute(executor);
	EXPECT_EQ(value->as<TypeInt::ValueType>(), 7410);

	(*value) += *TypeInt::create(85);
	EXPECT_EQ(value->as<TypeInt::ValueType>(), 7495);
	auto valueNotChanged = valueProvider->execute(executor);
	EXPECT_EQ(valueNotChanged->as<TypeInt::ValueType>(), 7410);
}

TEST_F(OperationsFixture, AddValues)
{
	auto valueAdder = loadOperation(R"( { "type" : "Add", "data" : 
		[ { "type" : "Read", "data" : "varTwo" }, { "type" : "Value", "data" : 93 } ] } )"_json);
	ASSERT_TRUE(bool(valueAdder));
	setTestVariables(74.1, 85);
	auto value = valueAdder->execute(executor);
	EXPECT_EQ(value->as<TypeInt::ValueType>(), 178);

	valueAdder = loadOperation(R"( { "type" : "Add", "data" : 
		[ { "type" : "Read", "data" : "varOne" }, { "type" : "Read", "data" : "varTwo" } ] } )"_json);
	ASSERT_TRUE(bool(valueAdder));
	value = valueAdder->execute(executor);
	EXPECT_EQ(value->as<TypeFloat::ValueType>(), 252.1);

	EXPECT_EQ(executor.getContext().get("varOne")->as<TypeFloat::ValueType>(), 252.1);
	EXPECT_EQ(executor.getContext().get("varTwo")->as<TypeInt::ValueType>(), 178);
}