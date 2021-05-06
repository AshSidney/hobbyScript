#include<gtest/gtest.h>

#include <CppScript/Operations.h>
#include <CppScript/Serializer.h>
#include <CppScript/Execution.h>
#include <CppScript/BasicTypes.h>

using namespace CppScript;

class OperationsFixture : public testing::Test
{
protected:
	std::vector<Operation::Ref> loadOperations(const Json& code)
	{
		JsonLoader codeData{ code };
		std::vector<Operation::Ref> operations;
		codeData.serialize(operations, "code");
		return operations;
	}

	Executor getExecutor(size_t dataSize)
	{
		Executor executor;
		executor.resize(dataSize);
		return executor;
	}

	void runOperations(Executor& executor, std::vector<Operation::Ref>& operations)
	{
		executor.pushCode(std::make_unique<CodeBlock>(operations));
		executor.run();
	}
};

TEST_F(OperationsFixture, Value)
{
	auto valueOperation = loadOperations(R"( { "code" : [ { "type" : "Value", "destIndex" : 0, "data" : 789.12 } ] } )"_json);
	auto executor = getExecutor(1);
	runOperations(executor, valueOperation);
	EXPECT_EQ(executor.get(0)->as<TypeFloat::ValueType>(), 789.12);

	*executor.get(0) += *TypeFloat::create(15.6);
	runOperations(executor, valueOperation);
	EXPECT_EQ(executor.get(0)->as<TypeFloat::ValueType>(), 804.72);
}

TEST_F(OperationsFixture, CloneValue)
{
	auto valueOperation = loadOperations(R"( { "code" : [ { "type" : "CloneValue", "destIndex" : 1, "data" : 456 } ] } )"_json);
	auto executor = getExecutor(3);
	runOperations(executor, valueOperation);
	EXPECT_EQ(executor.get(1)->as<TypeInt::ValueType>(), 456);

	*executor.get(1) += *TypeInt::create(23);
	runOperations(executor, valueOperation);
	EXPECT_EQ(executor.get(1)->as<TypeInt::ValueType>(), 456);
}

TEST_F(OperationsFixture, LocalValue)
{
	auto localValueReader = loadOperations(R"( { "code" : [ { "type" : "LocalValue", "destIndex" : 1, "sourceIndex" : 0 } ] } )"_json);
	auto executor = getExecutor(3);
	executor.set(0, TypeInt::create(753));
	executor.set(1, TypeFloat::create(45.67));
	runOperations(executor, localValueReader);
	EXPECT_EQ(executor.get(1)->as<TypeInt::ValueType>(), 753);

	*executor.get(1) += *TypeInt::create(12);
	EXPECT_EQ(executor.get(0)->as<TypeInt::ValueType>(), 765);
	EXPECT_EQ(executor.get(1)->as<TypeInt::ValueType>(), 765);
}

TEST_F(OperationsFixture, CloneLocalValue)
{
	auto localValueCloner = loadOperations(R"( { "code" : [ { "type" : "CloneLocalValue", "destIndex" : 0, "sourceIndex" : 1 } ] } )"_json);
	auto executor = getExecutor(3);
	executor.set(0, TypeInt::create(753));
	executor.set(1, TypeFloat::create(45.67));
	runOperations(executor, localValueCloner);
	EXPECT_EQ(executor.get(0)->as<TypeFloat::ValueType>(), 45.67);

	*executor.get(0) += *TypeInt::create(12);
	EXPECT_EQ(executor.get(0)->as<TypeFloat::ValueType>(), 57.67);
	EXPECT_EQ(executor.get(1)->as<TypeFloat::ValueType>(), 45.67);
}

TEST_F(OperationsFixture, Swap)
{
	auto swapper = loadOperations(R"( { "code" : [ { "type" : "Swap", "index0" : 0, "index1" : 2 } ] } )"_json);
	auto executor = getExecutor(3);
	executor.set(0, TypeInt::create(753));
	executor.set(2, TypeFloat::create(45.67));
	runOperations(executor, swapper);
	EXPECT_EQ(executor.get(0)->as<TypeFloat::ValueType>(), 45.67);
	EXPECT_EQ(executor.get(2)->as<TypeInt::ValueType>(), 753);
}

TEST_F(OperationsFixture, Add)
{
	auto valueAdder = loadOperations(R"( { "code" : [ { "type" : "Add", "destIndex" : 0, "sourceIndex" : 1 } ] } )"_json);
	auto executor = getExecutor(3);
	executor.set(0, TypeFloat::create(85.2));
	executor.set(1, TypeInt::create(-43));
	runOperations(executor, valueAdder);
	EXPECT_NEAR(executor.get(0)->as<TypeFloat::ValueType>(), 42.2, 1e-8);
	EXPECT_EQ(executor.get(1)->as<TypeInt::ValueType>(), -43);
}

TEST_F(OperationsFixture, CompareValues_Equal)
{
	auto valueCompare = loadOperations(R"( { "code" : [ { "type" : "Equal", "destIndex" : 0, "sourceIndex0" : 1, "sourceIndex1" : 2 } ] } )"_json);
	auto executor = getExecutor(3);
	executor.set(1, TypeFloat::create(54.0));
	executor.set(2, TypeInt::create(85));
	runOperations(executor, valueCompare);
	EXPECT_FALSE(executor.get(0)->as<TypeBool::ValueType>());

	executor.set(2, TypeInt::create(54));
	runOperations(executor, valueCompare);
	EXPECT_TRUE(executor.get(0)->as<TypeBool::ValueType>());

	executor.set(1, TypeFloat::create(87.1));
	executor.set(2, TypeInt::create(87));
	runOperations(executor, valueCompare);
	EXPECT_FALSE(executor.get(0)->as<TypeBool::ValueType>());
	EXPECT_EQ(executor.size(), 3);
}

TEST_F(OperationsFixture, CompareValues_NotEqual)
{
	auto valueCompare = loadOperations(R"( { "code" : [ { "type" : "NotEqual", "destIndex" : 1, "sourceIndex0" : 2, "sourceIndex1" : 0 } ] } )"_json);
	auto executor = getExecutor(3);
	executor.set(0, TypeFloat::create(54.0));
	executor.set(2, TypeInt::create(93));
	runOperations(executor, valueCompare);
	EXPECT_TRUE(executor.get(1)->as<TypeBool::ValueType>());

	executor.set(0, TypeFloat::create(93.0));
	executor.set(2, TypeInt::create(93));
	runOperations(executor, valueCompare);
	EXPECT_FALSE(executor.get(1)->as<TypeBool::ValueType>());
}

TEST_F(OperationsFixture, CompareValues_Less)
{
	auto valueCompare = loadOperations(R"( { "code" : [ { "type" : "Less", "destIndex" : 2, "sourceIndex0" : 1, "sourceIndex1" : 0 } ] } )"_json);
	auto executor = getExecutor(3);
	executor.set(0, TypeFloat::create(54.0));
	executor.set(1, TypeInt::create(93));
	runOperations(executor, valueCompare);
	EXPECT_FALSE(executor.get(2)->as<TypeBool::ValueType>());

	executor.set(0, TypeFloat::create(93.2));
	executor.set(1, TypeInt::create(87));
	runOperations(executor, valueCompare);
	EXPECT_TRUE(executor.get(2)->as<TypeBool::ValueType>());

	executor.set(0, TypeFloat::create(87.0));
	executor.set(1, TypeInt::create(87));
	runOperations(executor, valueCompare);
	EXPECT_FALSE(executor.get(2)->as<TypeBool::ValueType>());
}

TEST_F(OperationsFixture, CompareValues_Greater)
{
	auto valueCompare = loadOperations(R"( { "code" : [ { "type" : "Greater", "destIndex" : 2, "sourceIndex0" : 0, "sourceIndex1" : 1 } ] } )"_json);
	auto executor = getExecutor(3);
	executor.set(0, TypeInt::create(93));
	executor.set(1, TypeFloat::create(54.0));
	runOperations(executor, valueCompare);
	EXPECT_TRUE(executor.get(2)->as<TypeBool::ValueType>());

	executor.set(0, TypeInt::create(87));
	executor.set(1, TypeFloat::create(93.2));
	runOperations(executor, valueCompare);
	EXPECT_FALSE(executor.get(2)->as<TypeBool::ValueType>());

	executor.set(0, TypeInt::create(87));
	executor.set(1, TypeFloat::create(87.0));
	runOperations(executor, valueCompare);
	EXPECT_FALSE(executor.get(2)->as<TypeBool::ValueType>());
}

TEST_F(OperationsFixture, CompareValues_LessEqual)
{
	auto valueCompare = loadOperations(R"( { "code" : [ { "type" : "LessEqual", "destIndex" : 2, "sourceIndex0" : 0, "sourceIndex1" : 1 } ] } )"_json);
	auto executor = getExecutor(3);
	executor.set(0, TypeInt::create(93));
	executor.set(1, TypeFloat::create(54.0));
	runOperations(executor, valueCompare);
	EXPECT_FALSE(executor.get(2)->as<TypeBool::ValueType>());

	executor.set(0, TypeInt::create(87));
	executor.set(1, TypeFloat::create(93.2));
	runOperations(executor, valueCompare);
	EXPECT_TRUE(executor.get(2)->as<TypeBool::ValueType>());

	executor.set(0, TypeInt::create(87));
	executor.set(1, TypeFloat::create(87.0));
	runOperations(executor, valueCompare);
	EXPECT_TRUE(executor.get(2)->as<TypeBool::ValueType>());
}

TEST_F(OperationsFixture, CompareValues_GreaterEqual)
{
	auto valueCompare = loadOperations(R"( { "code" : [ { "type" : "GreaterEqual", "destIndex" : 2, "sourceIndex0" : 0, "sourceIndex1" : 1 } ] } )"_json);
	auto executor = getExecutor(3);
	executor.set(0, TypeInt::create(93));
	executor.set(1, TypeFloat::create(54.0));
	runOperations(executor, valueCompare);
	EXPECT_TRUE(executor.get(2)->as<TypeBool::ValueType>());

	executor.set(0, TypeInt::create(87));
	executor.set(1, TypeFloat::create(93.2));
	runOperations(executor, valueCompare);
	EXPECT_FALSE(executor.get(2)->as<TypeBool::ValueType>());

	executor.set(0, TypeInt::create(87));
	executor.set(1, TypeFloat::create(87.0));
	runOperations(executor, valueCompare);
	EXPECT_TRUE(executor.get(2)->as<TypeBool::ValueType>());
}


TEST_F(OperationsFixture, If_Condition)
{
	auto ifCond = loadOperations(R"( { "code" : [ { "type" : "If", "index" : 1, "then" : [ { "type" : "Value", "destIndex" : 0, "data" : 74 } ] } ] } )"_json);
	auto executor = getExecutor(2);
	executor.set(0, TypeFloat::create(54.7));
	executor.set(1, TypeBool::falseValue);
	runOperations(executor, ifCond);
	EXPECT_EQ(executor.get(0)->as<TypeFloat::ValueType>(), 54.7);

	executor.set(1, TypeBool::trueValue);
	runOperations(executor, ifCond);
	EXPECT_EQ(executor.get(0)->as<TypeInt::ValueType>(), 74);
}

TEST_F(OperationsFixture, IfElse_Condition)
{
	auto ifCond = loadOperations(R"( { "code" : [ { "type" : "IfElse", "index" : 1, "then" : [ { "type" : "Value", "destIndex" : 0, "data" : 47.1 } ],
		"else" : [ { "type" : "Value", "destIndex" : 0, "data" : 145 } ] } ] } )"_json);
	auto executor = getExecutor(3);
	executor.set(0, TypeFloat::create(54.0));
	executor.set(1, TypeBool::trueValue);
	runOperations(executor, ifCond);
	EXPECT_EQ(executor.get(0)->as<TypeFloat::ValueType>(), 47.1);

	executor.set(1, TypeBool::falseValue);
	runOperations(executor, ifCond);
	EXPECT_EQ(executor.get(0)->as<TypeInt::ValueType>(), 145);
}

TEST_F(OperationsFixture, LoopOperation)
{
	auto loopOperation = loadOperations(R"( { "code" : [ { "type" : "Loop", "code" : [ { "type" : "Add", "destIndex" : 1, "sourceIndex" : 3 },
		{ "type" : "Greater", "destIndex" : 4, "sourceIndex0" : 1, "sourceIndex1" : 2 }, { "type" : "If", "index" : 4, "then" : [ { "type" : "Break" } ] },
		{ "type" : "Add", "destIndex" : 0, "sourceIndex" : 1 } ] } ] } )"_json);
	auto executor = getExecutor(5);
	executor.set(0, TypeInt::create(0));
	executor.set(1, TypeInt::create(0));
	executor.set(2, TypeInt::create(10));
	executor.set(3, TypeInt::create(1));

	runOperations(executor, loopOperation);
	EXPECT_EQ(executor.get(0)->as<TypeInt::ValueType>(), 55);
	EXPECT_EQ(executor.get(1)->as<TypeInt::ValueType>(), 11);
}


long long fibonacci(size_t n)
{
	long long first = 0;
	long long second = 1;
	for (size_t i = 1; i < n; ++i)
	{
		long long next = first + second;
		first = second;
		second = next;
	}
	return second;
}

TEST_F(OperationsFixture, Fibonacci)
{
	auto fibonacciOperation = loadOperations(R"( { "code" : [ { "type" : "CloneValue", "destIndex" : 1, "data" : 0 }, { "type" : "CloneValue", "destIndex" : 2, "data" : 1 },
		{ "type" : "CloneValue", "destIndex" : 3, "data" : 1 }, { "type" : "Value", "destIndex" : 4, "data" : 1 },
		{ "type" : "Loop", "code" : [ { "type" : "Add", "destIndex" : 3, "sourceIndex" : 4 },
		{ "type" : "Greater", "destIndex" : 5, "sourceIndex0" : 3, "sourceIndex1" : 0 }, { "type" : "If", "index" : 5, "then" : [ { "type" : "Break" } ] },
		{ "type" : "Add", "destIndex" : 1, "sourceIndex" : 2 }, { "type" : "Swap", "index0" : 1, "index1" : 2 } ] } ] } )"_json);
	auto executor = getExecutor(6);
	executor.set(0, TypeInt::create(90));
	for (size_t i = 0; i < 10000; ++i)
		runOperations(executor, fibonacciOperation);

	EXPECT_EQ(executor.get(2)->as<TypeInt::ValueType>(), fibonacci(90));
}

TEST_F(OperationsFixture, Fibonacci2)
{
	long long rslt;
	for (size_t i = 0; i < 10000; ++i)
		rslt = fibonacci(90);

	EXPECT_EQ(rslt, fibonacci(90));
}