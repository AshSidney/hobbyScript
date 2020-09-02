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

TEST_F(OperationsFixture, SwapValues)
{
	auto variableSwapper = loadOperation(R"( { "type" : "Swap", "data" : [ "varTwo", "varOne" ] } )"_json);
	ASSERT_TRUE(bool(variableSwapper));
	setTestVariables(753.4, 951);
	auto value = variableSwapper->execute(executor);
	EXPECT_FALSE(bool(value));

	EXPECT_EQ(executor.getContext().get("varOne")->as<TypeInt::ValueType>(), 951);
	EXPECT_EQ(executor.getContext().get("varTwo")->as<TypeFloat::ValueType>(), 753.4);
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

TEST_F(OperationsFixture, CompareValues_Equal)
{
	auto valueCompare = loadOperation(R"( { "type" : "Equal", "data" : 
		[ { "type" : "Read", "data" : "varTwo" }, { "type" : "Value", "data" : 93 } ] } )"_json);
	ASSERT_TRUE(bool(valueCompare));
	setTestVariables(54.0, 54);
	EXPECT_FALSE(valueCompare->execute(executor)->as<TypeBool::ValueType>());

	valueCompare = loadOperation(R"( { "type" : "Equal", "data" : 
		[ { "type" : "Read", "data" : "varOne" }, { "type" : "Read", "data" : "varTwo" } ] } )"_json);
	ASSERT_TRUE(bool(valueCompare));
	EXPECT_TRUE(valueCompare->execute(executor)->as<TypeBool::ValueType>());

	valueCompare = loadOperation(R"( { "type" : "Equal", "data" : 
		[ { "type" : "Read", "data" : "varTwo" }, { "type" : "Read", "data" : "varOne" } ] } )"_json);
	ASSERT_TRUE(bool(valueCompare));
	setTestVariables(87.1, 87);
	EXPECT_FALSE(valueCompare->execute(executor)->as<TypeBool::ValueType>());

	valueCompare = loadOperation(R"( { "type" : "Equal", "data" : 
		[ { "type" : "Read", "data" : "varOne" }, { "type" : "Value", "data" : 87.1 } ] } )"_json);
	ASSERT_TRUE(bool(valueCompare));
	EXPECT_TRUE(valueCompare->execute(executor)->as<TypeBool::ValueType>());
}

TEST_F(OperationsFixture, CompareValues_NotEqual)
{
	auto valueCompare = loadOperation(R"( { "type" : "NotEqual", "data" : 
		[ { "type" : "Read", "data" : "varOne" }, { "type" : "Value", "data" : 93 } ] } )"_json);
	ASSERT_TRUE(bool(valueCompare));
	setTestVariables(54.0, 87);
	EXPECT_TRUE(valueCompare->execute(executor)->as<TypeBool::ValueType>());

	setTestVariables(93.0, 87);
	EXPECT_FALSE(valueCompare->execute(executor)->as<TypeBool::ValueType>());

	setTestVariables(93.2, 87);
	EXPECT_TRUE(valueCompare->execute(executor)->as<TypeBool::ValueType>());

	valueCompare = loadOperation(R"( { "type" : "NotEqual", "data" : 
		[ { "type" : "Read", "data" : "varOne" }, { "type" : "Read", "data" : "varTwo" } ] } )"_json);
	ASSERT_TRUE(bool(valueCompare));
	EXPECT_TRUE(valueCompare->execute(executor)->as<TypeBool::ValueType>());

	setTestVariables(87.0, 87);
	EXPECT_FALSE(valueCompare->execute(executor)->as<TypeBool::ValueType>());
}

TEST_F(OperationsFixture, CompareValues_Less)
{
	auto valueCompare = loadOperation(R"( { "type" : "Less", "data" : 
		[ { "type" : "Read", "data" : "varOne" }, { "type" : "Value", "data" : 93 } ] } )"_json);
	ASSERT_TRUE(bool(valueCompare));
	setTestVariables(54.7, 87);
	EXPECT_TRUE(valueCompare->execute(executor)->as<TypeBool::ValueType>());

	setTestVariables(93.0, 87);
	EXPECT_FALSE(valueCompare->execute(executor)->as<TypeBool::ValueType>());

	setTestVariables(93.2, 87);
	EXPECT_FALSE(valueCompare->execute(executor)->as<TypeBool::ValueType>());

	valueCompare = loadOperation(R"( { "type" : "Less", "data" : 
		[ { "type" : "Read", "data" : "varTwo" }, { "type" : "Read", "data" : "varOne" } ] } )"_json);
	ASSERT_TRUE(bool(valueCompare));
	EXPECT_TRUE(valueCompare->execute(executor)->as<TypeBool::ValueType>());

	setTestVariables(87.0, 456);
	EXPECT_FALSE(valueCompare->execute(executor)->as<TypeBool::ValueType>());
}

TEST_F(OperationsFixture, CompareValues_Greater)
{
	auto valueCompare = loadOperation(R"( { "type" : "Greater", "data" : 
		[ { "type" : "Read", "data" : "varOne" }, { "type" : "Value", "data" : 93 } ] } )"_json);
	ASSERT_TRUE(bool(valueCompare));
	setTestVariables(92.7, 87);
	EXPECT_FALSE(valueCompare->execute(executor)->as<TypeBool::ValueType>());

	setTestVariables(93.0, 87);
	EXPECT_FALSE(valueCompare->execute(executor)->as<TypeBool::ValueType>());

	setTestVariables(94.2, 87);
	EXPECT_TRUE(valueCompare->execute(executor)->as<TypeBool::ValueType>());

	valueCompare = loadOperation(R"( { "type" : "Greater", "data" : 
		[ { "type" : "Read", "data" : "varTwo" }, { "type" : "Read", "data" : "varOne" } ] } )"_json);
	ASSERT_TRUE(bool(valueCompare));
	EXPECT_FALSE(valueCompare->execute(executor)->as<TypeBool::ValueType>());

	setTestVariables(87.0, 456);
	EXPECT_TRUE(valueCompare->execute(executor)->as<TypeBool::ValueType>());
}

TEST_F(OperationsFixture, CompareValues_LessEqual)
{
	auto valueCompare = loadOperation(R"( { "type" : "LessEqual", "data" : 
		[ { "type" : "Read", "data" : "varOne" }, { "type" : "Value", "data" : 93 } ] } )"_json);
	ASSERT_TRUE(bool(valueCompare));
	setTestVariables(92.7, 87);
	EXPECT_TRUE(valueCompare->execute(executor)->as<TypeBool::ValueType>());

	setTestVariables(93.0, 87);
	EXPECT_TRUE(valueCompare->execute(executor)->as<TypeBool::ValueType>());

	setTestVariables(94.2, 87);
	EXPECT_FALSE(valueCompare->execute(executor)->as<TypeBool::ValueType>());

	valueCompare = loadOperation(R"( { "type" : "LessEqual", "data" : 
		[ { "type" : "Read", "data" : "varTwo" }, { "type" : "Read", "data" : "varOne" } ] } )"_json);
	ASSERT_TRUE(bool(valueCompare));
	EXPECT_TRUE(valueCompare->execute(executor)->as<TypeBool::ValueType>());

	setTestVariables(87.0, 456);
	EXPECT_FALSE(valueCompare->execute(executor)->as<TypeBool::ValueType>());
}

TEST_F(OperationsFixture, CompareValues_GreaterEqual)
{
	auto valueCompare = loadOperation(R"( { "type" : "GreaterEqual", "data" : 
		[ { "type" : "Read", "data" : "varOne" }, { "type" : "Value", "data" : 93 } ] } )"_json);
	ASSERT_TRUE(bool(valueCompare));
	setTestVariables(92.7, 87);
	EXPECT_FALSE(valueCompare->execute(executor)->as<TypeBool::ValueType>());

	setTestVariables(93.0, 87);
	EXPECT_TRUE(valueCompare->execute(executor)->as<TypeBool::ValueType>());

	setTestVariables(94.2, 87);
	EXPECT_TRUE(valueCompare->execute(executor)->as<TypeBool::ValueType>());

	valueCompare = loadOperation(R"( { "type" : "GreaterEqual", "data" : 
		[ { "type" : "Read", "data" : "varTwo" }, { "type" : "Read", "data" : "varOne" } ] } )"_json);
	ASSERT_TRUE(bool(valueCompare));
	EXPECT_FALSE(valueCompare->execute(executor)->as<TypeBool::ValueType>());

	setTestVariables(87.0, 456);
	EXPECT_TRUE(valueCompare->execute(executor)->as<TypeBool::ValueType>());
}
