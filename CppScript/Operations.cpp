#include <CppScript/Operations.h>
#include <CppScript/OperationsImpl.h>
#include <CppScript/Execution.h>
#include <CppScript/BasicTypes.h>
#include <CppScript/Serializer.h>
#include <array>


namespace CppScript
{

struct ArgumentMatcher
{
	ArgumentMatcher(const std::vector<Operation::Specification::ArgumentType>& operands)
		: currentArgument(operands.cbegin()), endArgument(operands.cend())
	{}

	enum class Result{Matched, Next, Failed};

	bool match(const Operation::Argument& argument)
	{
		Result result{ Result::Next };
		while (result == Result::Next)
			result = matchCurrent(argument);
		return result == Result::Matched;
	}

	bool isFinished() const
	{
		auto count = currentCount;
		for (auto it = currentArgument; it != endArgument; ++it)
		{
			if (it->minCount > count)
				return false;
			count = 0;
		}
		return true;
	}

	Result matchCurrent(const Operation::Argument& opType)
	{
		if (currentArgument == endArgument)
			return Result::Failed;
		if (currentArgument->typeId == nullptr || currentArgument->typeId == opType.typeId)
		{
			if (currentArgument->maxCount > currentCount)
			{
				++currentCount;
				return Result::Matched;
			}
		}
		else if (currentArgument->minCount < currentCount)
				return Result::Failed;
		++currentArgument;
		currentCount = 0;
		return Result::Next;
	}

	Operation::Specification::ArgumentTypes::const_iterator currentArgument;
	const Operation::Specification::ArgumentTypes::const_iterator endArgument;
	size_t currentCount{ 0 };
};


Operation::Specification::Specification(std::string&& operation, ArgumentTypes&& arguments, Creator&& creator)
	: arguments(std::move(arguments)), creator(std::move(creator))
{
	specPosition = specifications.emplace(std::move(operation), this);
}

Operation::Specification::~Specification()
{
	specifications.erase(specPosition);
}

Operation::Ref Operation::Specification::createOperation(const Operation::Call& opCall)
{
	auto [suitableSpecFirst, suitableSpecLast] = specifications.equal_range(opCall.operation);
	for (auto spec = suitableSpecFirst; spec != suitableSpecLast; ++spec)
	{
		if (!spec->second->matches(opCall))
			continue;
		auto operation = spec->second->creator(opCall.arguments);
		if (operation)
			return operation;
	}
	return {};
}

bool Operation::Specification::matches(const Operation::Call& opCall) const
{
	ArgumentMatcher matcher{ arguments };
	for (const auto& operand : opCall.arguments)
	{
		if (!matcher.match(operand))
			return false;
	}
	return matcher.isFinished();
}

std::unordered_multimap<std::string, const Operation::Specification*> Operation::Specification::specifications;


Operation::Ref Operation::create(const Operation::Call& opCall)
{
	return Specification::createOperation(opCall);
}


class MemoryAccessor
{
public:
	MemoryAccessor(Operation::Argument arg) : argument(arg)
	{}

	const Operation::Argument& get() const
	{
		return argument;
	}

	TypeBase::Ref& get(Executor& executor) const
	{
		return executor.get(argument.place);
	}

	void set(Executor& executor, TypeBase::Ref value) const
	{
		executor.set(argument.place, std::move(value));
	}

private:
	Operation::Argument argument;
};


void CopyOperationBase::execute(Executor& executor) const
{
	for (const auto& param : cloneParams)
		executor.set(param.target, executor.get(param.source)->clone());
}

void CopyOperationBase::addParams(const Argument& source, const Argument& target)
{
	cloneParams.push_back({ source.place, target.place });
}


Operation::Specification copySpecInt{ "copy", { {nullptr, 2, std::numeric_limits<int>::max()} }, CopyOperationBase::create<IntValue> };
Operation::Specification copySpecFloat{ "copy", { {nullptr, 2, std::numeric_limits<int>::max()} }, CopyOperationBase::create<FloatValue> };


template<> class CopyOperation<BoolValue> : public CopyOperationBase
{
public:
	void execute(Executor& executor) const override
	{
		CopyOperationBase::execute(executor);
	}

	void addParams(const Argument& source, const Argument& target) override
	{
		CopyOperationBase::addParams(source, target);
	}
};

Operation::Specification copySpecBool{ "copy", { {nullptr, 2, std::numeric_limits<int>::max()} }, CopyOperationBase::create<BoolValue> };


template <typename TARGET, typename SOURCE> class AddOperation : public Operation
{
public:
	AddOperation(const Argument& targetArg, const Argument& sourceArg)
		: source(sourceArg), target(targetArg)
	{}

	void execute(Executor& executor) const
	{
		static_cast<Type<TARGET>&>(*target.get(executor)) += static_cast<Type<SOURCE>&>(*source.get(executor));
	}

	static Ref create(const Arguments& arguments)
	{
		return std::make_unique<AddOperation>(arguments[0], arguments[1]);
	}

private:
	MemoryAccessor source;
	MemoryAccessor target;
};

Operation::Specification addSpecIntInt{ "+=", {{&TypeInt::id(), 2, 2}}, AddOperation<IntValue, IntValue>::create };
Operation::Specification addSpecFloatFloat{ "+=", {{&TypeFloat::id(), 2, 2}}, AddOperation<FloatValue, FloatValue>::create };
Operation::Specification addSpecFloatInt{ "+=", {{&TypeFloat::id(), 1, 1}, {&TypeInt::id(), 1, 1}}, AddOperation<FloatValue, IntValue>::create };


class SwapReferenceOperation : public Operation
{
public:
	SwapReferenceOperation(const Argument& arg0, const Argument& arg1)
		: operand0(arg0), operand1(arg1)
	{}

	void execute(Executor& executor) const
	{
		executor.swap(operand0.get().place, operand1.get().place);
	}

	static Ref create(const Arguments& arguments)
	{
		return std::make_unique<SwapReferenceOperation>(arguments[0], arguments[1]);
	}

private:
	MemoryAccessor operand0;
	MemoryAccessor operand1;
};

Operation::Specification swapRefSpec{ "SwapRef", {{nullptr, 2, 2}}, SwapReferenceOperation::create };


// deprecated

/*bool operator==(const Operation::OperandType& left, const Operation::OperandType& right)
{
	return left.typeId == right.typeId && left.source == right.source;
}

bool operator!=(const Operation::OperandType& left, const Operation::OperandType& right)
{
	return !(left == right);
}*/

OperandSourceOld operator&(OperandSourceOld first, OperandSourceOld second)
{
	return OperandSourceOld(size_t(first) & size_t(second));
}

OperandSourceOld operator|(OperandSourceOld first, OperandSourceOld second)
{
	return OperandSourceOld(size_t(first) | size_t(second));
}

class OperationCreator
{
public:
	virtual OperationOld::Ref create() const = 0;
	virtual const std::string& getName() const = 0;

	using Creators = std::array<OperationCreator*, size_t(OperationType::Last)>;
	static Creators creators;
	
	using Names = std::unordered_map<std::string, OperationType>;
	static Names names;
};

OperationCreator::Creators OperationCreator::creators{ nullptr };
OperationCreator::Names OperationCreator::names;


template <class O> class OpCreator : public OperationCreator
{
public:
	constexpr OpCreator(const char* opName) : name(opName)
	{
		creators[size_t(O::getOpType())] = this;
		names[name] = O::getOpType();
	}

	virtual OperationOld::Ref create() const override
	{
		return std::make_unique<O>();
	}

	virtual const std::string& getName() const override
	{
		return name;
	}

private:
	std::string name;
};

OpCreator<ValueOperation> valueOp{ "Value" };
OpCreator<CloneValueOperation> cloneValueOp{ "CloneValue" };
OpCreator<LocalValueOperation> localValueOp{ "LocalValue" };
OpCreator<CloneLocalValueOperation> cloneLocalValueOp{ "CloneLocalValue" };
OpCreator<SwapOperation> swapOp{ "Swap" };
OpCreator<AddOperationOld> addOp{ "Add" };
OpCreator<EqualOperation> equalOp{ "Equal" };
OpCreator<NotEqualOperation> notEqualOp{ "NotEqual" };
OpCreator<LessOperation> lessOp{ "Less" };
OpCreator<GreaterOperation> greaterOp{ "Greater" };
OpCreator<LessEqualOperation> lessEqualOp{ "LessEqual" };
OpCreator<GreaterEqualOperation> greaterEqualOp{ "GreaterEqual" };
OpCreator<JumpOperation> jumpOp{ "Jump" };
OpCreator<JumpIfOperation> jumpIfOp{ "JumpIf" };
OpCreator<IfOperation> ifOp{ "If" };
OpCreator<IfElseOperation> ifElseOp{ "IfElse" };
//OpCreator<LoopOperation> loopOp{ "Loop" };
//OpCreator<BreakOperation> breakOp{ "Break" };


OperationOld::Ref OperationOld::create(OperationType opType)
{
	return OperationCreator::creators[size_t(opType)]->create();
}

OperationOld::Ref OperationOld::create(const std::string& opType)
{
	return create(OperationCreator::names.at(opType));
}

const std::string& OperationOld::getName(OperationType opType)
{
	return OperationCreator::creators[size_t(opType)]->getName();
}


void ValueOperationBase::serialize(Serializer& serializer)
{
	serializer.serialize(destinationIndex, "destIndex");
}


void DirectValueOperationBase::serialize(Serializer& serializer)
{
	ValueOperationBase::serialize(serializer);
	serializer.serialize(value, "data");
}


void ValueOperation::execute(ExecutorOld& executor) const
{
	executor.set(destinationIndex, value);
}


void CloneValueOperation::execute(ExecutorOld& executor) const
{
	executor.set(destinationIndex, value->clone());
}


void LocalValueOperationBase::serialize(Serializer& serializer)
{
	ValueOperationBase::serialize(serializer);
	serializer.serialize(sourceIndex, "sourceIndex");
}


void LocalValueOperation::execute(ExecutorOld& executor) const
{
	executor.set(destinationIndex, executor.get(sourceIndex));
}


void CloneLocalValueOperation::execute(ExecutorOld& executor) const
{
	executor.set(destinationIndex, executor.get(sourceIndex)->clone());
}


void SwapOperation::execute(ExecutorOld& executor) const
{
	std::swap(executor.get(index0), executor.get(index1));
}

void SwapOperation::serialize(Serializer& serializer)
{
	serializer.serialize(index0, "index0");
	serializer.serialize(index1, "index1");
	opType = getType();
}


void AccumulationOperation::serialize(Serializer& serializer)
{
	serializer.serialize(destinationIndex, "destIndex");
	serializer.serialize(sourceIndex, "sourceIndex");
}


void AddOperationOld::execute(ExecutorOld& executor) const
{
	*executor.get(destinationIndex) += *executor.get(sourceIndex);
}


void ComparisonOperation::serialize(Serializer& serializer)
{
	serializer.serialize(destinationIndex, "destIndex");
	serializer.serialize(sourceIndex0, "sourceIndex0");
	serializer.serialize(sourceIndex1, "sourceIndex1");
}


void EqualOperation::execute(ExecutorOld& executor) const
{
	executor.set(destinationIndex, *executor.get(sourceIndex0) == *executor.get(sourceIndex1) ? TypeBool::trueValue : TypeBool::falseValue);
}

void NotEqualOperation::execute(ExecutorOld& executor) const
{
	executor.set(destinationIndex, *executor.get(sourceIndex0) == *executor.get(sourceIndex1) ? TypeBool::falseValue : TypeBool::trueValue);
}

void LessOperation::execute(ExecutorOld& executor) const
{
	executor.set(destinationIndex, *executor.get(sourceIndex0) < *executor.get(sourceIndex1) ? TypeBool::trueValue : TypeBool::falseValue);
}

void GreaterOperation::execute(ExecutorOld& executor) const
{
	executor.set(destinationIndex, *executor.get(sourceIndex1) < *executor.get(sourceIndex0) ? TypeBool::trueValue : TypeBool::falseValue);
}

void LessEqualOperation::execute(ExecutorOld& executor) const
{
	executor.set(destinationIndex, *executor.get(sourceIndex1) < *executor.get(sourceIndex0) ? TypeBool::falseValue : TypeBool::trueValue);
}

void GreaterEqualOperation::execute(ExecutorOld& executor) const
{
	executor.set(destinationIndex, *executor.get(sourceIndex0) < *executor.get(sourceIndex1) ? TypeBool::falseValue : TypeBool::trueValue);
}


void JumpOperation::execute(ExecutorOld& executor) const
{
	executor.getCode().setNextOperation(nextOperation);
}

void JumpOperation::serialize(Serializer& serializer)
{
	serializer.serialize(nextOperation, "to");
	opType = getType();
}

void JumpIfOperation::execute(ExecutorOld& executor) const
{
	if (executor.get(index)->as<TypeBool::ValueType>())
		executor.getCode().setNextOperation(nextOperation);
}

void JumpIfOperation::serialize(Serializer& serializer)
{
	serializer.serialize(index, "index");
	serializer.serialize(nextOperation, "to");
	opType = getType();
}


void IfOperation::execute(ExecutorOld& executor) const
{
	if (executor.get(index)->as<TypeBool::ValueType>())
		executor.pushCode(operations);
}

void IfOperation::serialize(Serializer& serializer)
{
	serializer.serialize(index, "index");
	serializer.serialize(operations, "then");
	operations.push_back(std::make_unique<CodeBlock::BlockEndOperation>());
	opType = getType();
}

void IfElseOperation::execute(ExecutorOld& executor) const
{
	executor.pushCode(executor.get(index)->as<TypeBool::ValueType>() ? trueOperations : falseOperations);
}

void IfElseOperation::serialize(Serializer& serializer)
{
	serializer.serialize(index, "index");
	serializer.serialize(trueOperations, "then");
	serializer.serialize(falseOperations, "else");
	trueOperations.push_back(std::make_unique<CodeBlock::BlockEndOperation>());
	falseOperations.push_back(std::make_unique<CodeBlock::BlockEndOperation>());
	opType = getType();
}


/*class LoopBlock : public CodeBlock
{
public:
	explicit LoopBlock(const std::vector<Operation::Ref>& code) : CodeBlock(code)
	{}

	const Operation& getOperation() override
	{
		size_t currOperation = currentOperation++;
		currentOperation %= operations.size();
		return *operations[currOperation];
	}

	BlockType getType() const override
	{
		return BlockType::Loop;
	}
};


void LoopOperation::execute(Executor& executor) const
{
	executor.pushCode(std::make_unique<LoopBlock>(operations));
}

void LoopOperation::serialize(Serializer& serializer)
{
	serializer.serialize(operations, "code");
}


void BreakOperation::execute(Executor& executor) const
{
	std::unique_ptr<CodeBlock> loopBlock;
	while (executor.sizeCode() > 0)
	{
		loopBlock = executor.popCode();
		if (loopBlock->getType() == BlockType::Loop)
			break;
	}
	executor.resize(loopBlock->getBottom());
}*/

}