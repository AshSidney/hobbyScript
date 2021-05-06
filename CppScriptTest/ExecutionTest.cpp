#include<gtest/gtest.h>

#include <CppScript/Operations.h>
#include <CppScript/Serializer.h>
#include <CppScript/Execution.h>
#include <CppScript/BasicTypes.h>

using namespace CppScript;

class ExecutionFixture : public testing::Test
{
protected:
	Operation::Ref loadOperation(const Json& opData)
	{
		JsonLoader data{ opData };
		Operation::Ref operation;
		data.serialize(operation);
		return operation;
	}

	Context context;
	Executor executor{ context };
};

TEST_F(ExecutionFixture, Value)
{
	auto valueProvider = loadOperation(R"( { "type" : "Value", "data" : 789.12 } )"_json);
	ASSERT_TRUE(bool(valueProvider));
	valueProvider->execute(executor);
	EXPECT_EQ(context.top()->as<TypeFloat::ValueType>(), 789.12);

	*context.top() += *TypeFloat::create(15.6);
	valueProvider->execute(executor);
	EXPECT_EQ(context.top()->as<TypeFloat::ValueType>(), 804.72);
}
TEST_F(OperationsFixture, CloneValue)
{
	auto valueProvider = loadOperation(R"( { "type" : "CloneValue", "data" : 456 } )"_json);
	ASSERT_TRUE(bool(valueProvider));
	valueProvider->execute(executor);
	EXPECT_EQ(context.top()->as<TypeInt::ValueType>(), 456);

	*context.top() += *TypeInt::create(23);
	valueProvider->execute(executor);
	EXPECT_EQ(context.top()->as<TypeInt::ValueType>(), 456);
}

TEST_F(OperationsFixture, LocalValue)
{
	auto localValueReader = loadOperation(R"( { "type" : "LocalValue", "data" : 0 } )"_json);
	ASSERT_TRUE(bool(localValueReader));
	context.push(TypeInt::create(753));
	context.push(TypeFloat::create(45.67));
	localValueReader->execute(executor);
	EXPECT_EQ(context.top()->as<TypeInt::ValueType>(), 753);

	*context.top() += *TypeInt::create(12);
	EXPECT_EQ(context.top()->as<TypeInt::ValueType>(), 765);
	EXPECT_EQ(context.get(0)->as<TypeInt::ValueType>(), 765);
}

TEST_F(OperationsFixture, CloneLocalValue)
{
	auto localValueCloner = loadOperation(R"( { "type" : "CloneLocalValue", "data" : 1 } )"_json);
	ASSERT_TRUE(bool(localValueCloner));
	context.push(TypeInt::create(753));
	context.push(TypeFloat::create(45.67));
	localValueCloner->execute(executor);
	EXPECT_EQ(context.top()->as<TypeFloat::ValueType>(), 45.67);

	*context.top() += *TypeInt::create(12);
	EXPECT_EQ(context.top()->as<TypeFloat::ValueType>(), 57.67);
	EXPECT_EQ(context.get(1)->as<TypeFloat::ValueType>(), 45.67);
}

TEST_F(OperationsFixture, SwapTop)
{
	auto swapper = loadOperation(R"( { "type" : "SwapTop" } )"_json);
	ASSERT_TRUE(bool(swapper));
	context.push(TypeInt::create(753));
	context.push(TypeFloat::create(45.67));
	swapper->execute(executor);
	EXPECT_EQ(context.top()->as<TypeInt::ValueType>(), 753);
	EXPECT_EQ(context.top(1)->as<TypeFloat::ValueType>(), 45.67);
}

TEST_F(OperationsFixture, Add)
{
	auto valueAdder = loadOperation(R"( { "type" : "Add" } )"_json);
	ASSERT_TRUE(bool(valueAdder));
	context.push(TypeFloat::create(486.2));
	context.push(TypeInt::create(-43));
	context.push(TypeFloat::create(72.4));
	valueAdder->execute(executor);
	EXPECT_NEAR(context.top()->as<TypeFloat::ValueType>(), 29.4, 1e-8);

	valueAdder->execute(executor);
	EXPECT_NEAR(context.top()->as<TypeFloat::ValueType>(), 515.6, 1e-8);
}

/*TEST_F(OperationsFixture, AssignValue)
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


TEST_F(OperationsFixture, SequenceOperations)
{
	auto sequence = loadOperation(R"( { "type" : "Sequence", "data" : 
		[ { "type" : "Assign", "data" : [ "varTwo", { "type" : "Read", "data" : "varOne" } ] },
		  { "type" : "Add", "data" : [ { "type" : "Read", "data" : "varOne" }, { "type" : "Value", "data" : 4.7 } ] } ] } )"_json);
	ASSERT_TRUE(bool(sequence));
	setTestVariables(15.9, 42);
	EXPECT_EQ(sequence->execute(executor)->as<TypeFloat::ValueType>(), 20.6);
	EXPECT_EQ(context.get("varOne")->as<TypeFloat::ValueType>(), 20.6);
	EXPECT_EQ(context.get("varTwo")->as<TypeFloat::ValueType>(), 15.9);
}*/
