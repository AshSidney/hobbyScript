#include<gtest/gtest.h>
#include <gmock/gmock.h>

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
		operations.push_back(std::make_unique<CodeBlock::BlockEndOperation>());
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
		executor.pushCode(operations);
		executor.run();
	}
};

class OperationMock : public Operation
{
public:
	MOCK_METHOD(void, execute, (Executor&), (const, override));
	MOCK_METHOD(void, serialize, (Serializer&), (override));
	//MOCK_METHOD(std::vector<TypePlace>, getChangedTypes, (), (const, override));
};

TEST_F(OperationsFixture, MatchSignatureToSpecificationBasic)
{
	Operation::Specification opSpec{ {{&TypeFloat::id(), OperandSource::DirectValue}} };
	EXPECT_TRUE(opSpec.matches({ "testOp", {{TypeFloat::id(), OperandSource::DirectValue}} }));
	EXPECT_FALSE(opSpec.matches({ "testOp", {{TypeFloat::id(), OperandSource::MemoryValue}} }));
	EXPECT_FALSE(opSpec.matches({ "testOp", {{TypeInt::id(), OperandSource::MemoryValue}} }));
	EXPECT_FALSE(opSpec.matches({ "testOp", {} }));
	EXPECT_FALSE(opSpec.matches({ "testOp", {{TypeFloat::id(), OperandSource::DirectValue}, {TypeInt::id(), OperandSource::MemoryValue}} }));
}

TEST_F(OperationsFixture, MatchSignatureToSpecificationVariadic)
{
	Operation::Specification opSpec{ {{&TypeInt::id(), OperandSource::MemoryValue, 1, 2}} };
	EXPECT_TRUE(opSpec.matches({ "testOp", {{TypeInt::id(), OperandSource::MemoryValue}} }));
	EXPECT_TRUE(opSpec.matches({ "testOp", {{TypeInt::id(), OperandSource::MemoryValue}, {TypeInt::id(), OperandSource::MemoryValue}} }));
	EXPECT_FALSE(opSpec.matches({ "testOp", {{TypeFloat::id(), OperandSource::MemoryValue}} }));
	EXPECT_FALSE(opSpec.matches({ "testOp", {{TypeInt::id(), OperandSource::MemoryValue}, {TypeInt::id(), OperandSource::DirectValue}} }));
	EXPECT_FALSE(opSpec.matches({ "testOp", {{TypeInt::id(), OperandSource::MemoryValue}, {TypeInt::id(), OperandSource::MemoryValue}, {TypeInt::id(), OperandSource::MemoryValue}} }));
}

TEST_F(OperationsFixture, AddCustomOperations)
{
	Operation::Ref operation1 = std::make_unique<OperationMock>();
	auto operation1Ptr = operation1.get();
	Operation::add("TestOp1", {{{nullptr, OperandSource::DirectValue, 1, 3}},
		[&](const std::vector<Operation::OperandType>&) { return std::move(operation1); } });

	Operation::Ref operation2 = std::make_unique<OperationMock>();
	auto operation2Ptr = operation2.get();
	Operation::add("TestOp2", {{{ &TypeInt::id(), OperandSource::MemoryValue }},
		[&](const std::vector<Operation::OperandType>&) { return std::move(operation2); } });

	auto createdOperation = Operation::create({ "TestOp1", { { TypeFloat::id(), OperandSource::DirectValue } } });
	EXPECT_EQ(createdOperation.get(), operation1Ptr);
	operation1 = std::move(createdOperation);
	EXPECT_FALSE(Operation::create({ "TestOp1", { { TypeFloat::id(), OperandSource::MemoryValue } } }));
	EXPECT_EQ(Operation::create({ "TestOp1", { { TypeFloat::id(), OperandSource::DirectValue}, { TypeBool::id(), OperandSource::DirectValue } } }).get(), operation1Ptr);

	EXPECT_FALSE(Operation::create({ "TestOp2", { { TypeInt::id(), OperandSource::DirectValue } } }));
	EXPECT_EQ(Operation::create({ "TestOp2", { { TypeInt::id(), OperandSource::MemoryValue } } }).get(), operation2Ptr);
	EXPECT_FALSE(Operation::create({ "TestOp2", { { TypeFloat::id(), OperandSource::MemoryValue } } }));

	EXPECT_FALSE(Operation::create({ "NonExistingOp", { { TypeInt::id(), OperandSource::DirectValue } } }));
}

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

/*TEST_F(OperationsFixture, LoopOperation)
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
}*/


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

long long fibonacciRecurse(size_t n)
{
	if (n <= 1)
		return n;
	return fibonacciRecurse(n - 1) + fibonacciRecurse(n - 2);
}

/*TEST_F(OperationsFixture, Fibonacci)
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
}*/

TEST_F(OperationsFixture, FibonacciJump)
{
	auto fibonacciOperation = loadOperations(R"( { "code" : [ { "type" : "CloneValue", "destIndex" : 1, "data" : 0 }, { "type" : "CloneValue", "destIndex" : 2, "data" : 1 },
		{ "type" : "CloneValue", "destIndex" : 3, "data" : 1 }, { "type" : "Value", "destIndex" : 4, "data" : 1 },
		{ "type" : "Add", "destIndex" : 1, "sourceIndex" : 2 }, { "type" : "Swap", "index0" : 1, "index1" : 2 },
		{ "type" : "Add", "destIndex" : 3, "sourceIndex" : 4 },	{ "type" : "Greater", "destIndex" : 5, "sourceIndex0" : 0, "sourceIndex1" : 3 },
		{ "type" : "JumpIf", "index" : 5, "to" : 4 } ] } )"_json);
	auto executor = getExecutor(6);
	executor.set(0, TypeInt::create(90));
	for (size_t i = 0; i < 10000; ++i)
		runOperations(executor, fibonacciOperation);

	EXPECT_EQ(executor.get(2)->as<TypeInt::ValueType>(), fibonacci(90));
}

TEST_F(OperationsFixture, FibonacciJumpIt)
{
	auto fibonacciOperation = loadOperations(R"( { "code" : [ { "type" : "CloneValue", "destIndex" : 1, "data" : 0 }, { "type" : "CloneValue", "destIndex" : 2, "data" : 1 },
		{ "type" : "CloneValue", "destIndex" : 3, "data" : 1 }, { "type" : "Value", "destIndex" : 4, "data" : 1 },
		{ "type" : "Add", "destIndex" : 1, "sourceIndex" : 2 }, { "type" : "Swap", "index0" : 1, "index1" : 2 },
		{ "type" : "Add", "destIndex" : 3, "sourceIndex" : 4 },	{ "type" : "Greater", "destIndex" : 5, "sourceIndex0" : 0, "sourceIndex1" : 3 },
		{ "type" : "JumpIf", "index" : 5, "to" : 4 } ] } )"_json);
	auto executor = getExecutor(6);
	executor.set(0, TypeInt::create(90));
	for (size_t i = 0; i < 10000; ++i)
	{
		executor.pushCode(fibonacciOperation);
		executor.runIt();
	}

	EXPECT_EQ(executor.get(2)->as<TypeInt::ValueType>(), fibonacci(90));
}

TEST_F(OperationsFixture, FibonacciDirectExec)
{
	auto fibonacciOperation = loadOperations(R"( { "code" : [ { "type" : "CloneValue", "destIndex" : 1, "data" : 0 }, { "type" : "CloneValue", "destIndex" : 2, "data" : 1 },
		{ "type" : "CloneValue", "destIndex" : 3, "data" : 1 }, { "type" : "Value", "destIndex" : 4, "data" : 1 },
		{ "type" : "Add", "destIndex" : 1, "sourceIndex" : 2 }, { "type" : "Swap", "index0" : 1, "index1" : 2 },
		{ "type" : "Add", "destIndex" : 3, "sourceIndex" : 4 },	{ "type" : "Greater", "destIndex" : 5, "sourceIndex0" : 0, "sourceIndex1" : 3 },
		{ "type" : "JumpIf", "index" : 5, "to" : 4 } ] } )"_json);
	auto executor = getExecutor(6);
	executor.set(0, TypeInt::create(90));
	executor.pushCode(fibonacciOperation);
	for (size_t i = 0; i < 10000; ++i)
	{
		executor.runFib();
	}

	EXPECT_EQ(executor.get(2)->as<TypeInt::ValueType>(), fibonacci(90));
}

TEST_F(OperationsFixture, FibonacciDirectExecNC)
{
	auto fibonacciOperation = loadOperations(R"( { "code" : [ { "type" : "CloneValue", "destIndex" : 1, "data" : 0 }, { "type" : "CloneValue", "destIndex" : 2, "data" : 1 },
		{ "type" : "CloneValue", "destIndex" : 3, "data" : 1 }, { "type" : "Value", "destIndex" : 4, "data" : 1 },
		{ "type" : "Add", "destIndex" : 1, "sourceIndex" : 2 }, { "type" : "Swap", "index0" : 1, "index1" : 2 },
		{ "type" : "Add", "destIndex" : 3, "sourceIndex" : 4 },	{ "type" : "Greater", "destIndex" : 5, "sourceIndex0" : 0, "sourceIndex1" : 3 },
		{ "type" : "JumpIf", "index" : 5, "to" : 4 } ] } )"_json);
	auto executor = getExecutor(6);
	executor.set(0, TypeInt::create(90));
	executor.pushCode(fibonacciOperation);
	for (size_t i = 0; i < 10000; ++i)
	{
		executor.runFibNC();
	}

	EXPECT_EQ(executor.get(2)->as<TypeInt::ValueType>(), fibonacci(90));
}

TEST_F(OperationsFixture, FibonacciDirectExecNOP)
{
	auto fibonacciOperation = loadOperations(R"( { "code" : [ { "type" : "CloneValue", "destIndex" : 1, "data" : 0 }, { "type" : "CloneValue", "destIndex" : 2, "data" : 1 },
		{ "type" : "CloneValue", "destIndex" : 3, "data" : 1 }, { "type" : "Value", "destIndex" : 4, "data" : 1 },
		{ "type" : "Add", "destIndex" : 1, "sourceIndex" : 2 }, { "type" : "Swap", "index0" : 1, "index1" : 2 },
		{ "type" : "Add", "destIndex" : 3, "sourceIndex" : 4 },	{ "type" : "Greater", "destIndex" : 5, "sourceIndex0" : 0, "sourceIndex1" : 3 },
		{ "type" : "JumpIf", "index" : 5, "to" : 4 } ] } )"_json);
	auto executor = getExecutor(6);
	executor.set(0, TypeInt::create(90));
	for (size_t i = 0; i < 10000; ++i)
	{
		executor.runFibNOP();
	}

	EXPECT_EQ(executor.get(2)->as<TypeInt::ValueType>(), fibonacci(90));
}

TEST_F(OperationsFixture, FibonacciDirectExecNOP2)
{
	auto fibonacciOperation = loadOperations(R"( { "code" : [ { "type" : "CloneValue", "destIndex" : 1, "data" : 0 }, { "type" : "CloneValue", "destIndex" : 2, "data" : 1 },
		{ "type" : "CloneValue", "destIndex" : 3, "data" : 1 }, { "type" : "Value", "destIndex" : 4, "data" : 1 },
		{ "type" : "Add", "destIndex" : 1, "sourceIndex" : 2 }, { "type" : "Swap", "index0" : 1, "index1" : 2 },
		{ "type" : "Add", "destIndex" : 3, "sourceIndex" : 4 },	{ "type" : "Greater", "destIndex" : 5, "sourceIndex0" : 0, "sourceIndex1" : 3 },
		{ "type" : "JumpIf", "index" : 5, "to" : 4 } ] } )"_json);
	auto executor = getExecutor(6);
	executor.set(0, TypeInt::create(90));
	for (size_t i = 0; i < 10000; ++i)
	{
		executor.runFibNOP2();
	}

	EXPECT_EQ(executor.get(2)->as<TypeInt::ValueType>(), fibonacci(90));
}

TEST_F(OperationsFixture, FibonacciDirectExecNOP3)
{
	auto fibonacciOperation = loadOperations(R"( { "code" : [ { "type" : "CloneValue", "destIndex" : 1, "data" : 0 }, { "type" : "CloneValue", "destIndex" : 2, "data" : 1 },
		{ "type" : "CloneValue", "destIndex" : 3, "data" : 1 }, { "type" : "Value", "destIndex" : 4, "data" : 1 },
		{ "type" : "Add", "destIndex" : 1, "sourceIndex" : 2 }, { "type" : "Swap", "index0" : 1, "index1" : 2 },
		{ "type" : "Add", "destIndex" : 3, "sourceIndex" : 4 },	{ "type" : "Greater", "destIndex" : 5, "sourceIndex0" : 0, "sourceIndex1" : 3 },
		{ "type" : "JumpIf", "index" : 5, "to" : 4 } ] } )"_json);
	auto executor = getExecutor(6);
	executor.set(0, TypeInt::create(90));
	for (size_t i = 0; i < 10000; ++i)
	{
		executor.runFibNOP3();
	}

	EXPECT_EQ(executor.get(2)->as<TypeInt::ValueType>(), fibonacci(90));
}


/*TEST_F(OperationsFixture, NOFibonacci)
{
	auto fibonacciOperation = loadOperations(R"( { "code" : [ { "type" : "CloneValue", "destIndex" : 1, "sourceType" : "int", "value" : 0 },
		{ "type" : "CloneValue", "destIndex" : 2, "sourceType" : "int", "value" : 1 },
		{ "type" : "Add", "destType" : "int", "destIndex" : 0, "sourceType" : "int", "value" : -1 },
		{ "type" : "Add", "destType" : "int", "destIndex" : 1, "sourceType" : "int", "index" : 2 }, { "type" : "Swap", "index0" : 1, "index1" : 2 },
		{ "type" : "Add", "destType" : "int", "destIndex" : 0, "sourceType" : "int", "value" : -1 },
		{ "type" : "JumpPositive", "sourceType" : "int", "index" : 0, "to" : 3 } ] } )"_json);
	auto executor = getExecutor(6);
	executor.set(0, TypeInt::create(90));
	for (size_t i = 0; i < 10000; ++i)
	{
		executor.pushCode(fibonacciOperation);
		executor.runIt();
	}

	EXPECT_EQ(executor.get(2)->as<TypeInt::ValueType>(), fibonacci(90));
}*/

TEST_F(OperationsFixture, FibonacciCpp)
{
	std::vector<long long> rslt;
	for (size_t i = 0; i < 10000; ++i)
		rslt.push_back(fibonacci(90));

	EXPECT_EQ(rslt[0], fibonacci(90));
}

TEST_F(OperationsFixture, FibonacciRecurseCpp)
{
	std::vector<long long> rslt;
	for (size_t i = 0; i < 1; ++i)
		rslt.push_back(fibonacciRecurse(40));

	EXPECT_EQ(rslt[0], fibonacci(40));
}