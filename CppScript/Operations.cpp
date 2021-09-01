#include <CppScript/Operations.h>
#include <CppScript/Serializer.h>
#include <CppScript/Execution.h>
#include <CppScript/BasicTypes.h>
#include <array>


namespace CppScript
{

/*bool operator==(const Operation::OperandType& left, const Operation::OperandType& right)
{
	return left.typeId == right.typeId && left.source == right.source;
}

bool operator!=(const Operation::OperandType& left, const Operation::OperandType& right)
{
	return !(left == right);
}*/

OperandSource operator&(OperandSource first, OperandSource second)
{
	return OperandSource(size_t(first) & size_t(second));
}

OperandSource operator|(OperandSource first, OperandSource second)
{
	return OperandSource(size_t(first) | size_t(second));
}

struct OperandMatcher
{
	OperandMatcher(const std::vector<Operation::Specification::VariadicTypeOperand>& operands)
		: currentOperand(operands.cbegin()), endOperand(operands.cend())
	{}

	enum class Result{Matched, Next, Failed};

	bool match(const Operation::OperandType& opType)
	{
		Result result{ Result::Next };
		while (result == Result::Next)
			result = matchCurrent(opType);
		return result == Result::Matched;
	}

	bool isFinished() const
	{
		return currentOperand == endOperand || std::next(currentOperand) == endOperand
			&& currentOperand->minCount <= currentCount && currentOperand->maxCount >= currentCount;
	}

	Result matchCurrent(const Operation::OperandType& opType)
	{
		if (currentOperand == endOperand)
			return Result::Failed;
		if ((currentOperand->typeId == nullptr || *currentOperand->typeId == opType.typeId)
			&& (currentOperand->source & opType.source) != OperandSource::Invalid)
		{
			if (currentOperand->maxCount > currentCount)
			{
				++currentCount;
				return Result::Matched;
			}
		}
		else if (currentOperand->minCount < currentCount)
				return Result::Failed;
		++currentOperand;
		currentCount = 0;
		return Result::Next;
	}

	decltype(Operation::Specification::operands)::const_iterator currentOperand;
	const decltype(Operation::Specification::operands)::const_iterator endOperand;
	size_t currentCount{ 0 };
};

bool Operation::Specification::matches(const Operation::Signature& opSign) const
{
	OperandMatcher matcher{ operands };
	for (const auto& operand : opSign.operands)
	{
		if (!matcher.match(operand))
			return false;
	}
	return matcher.isFinished();
}

std::unordered_multimap<std::string, Operation::Specification> Operation::creators;

Operation::Ref Operation::create(const Operation::Signature& opSign)
{
	auto suitableCreators = creators.equal_range(opSign.operation);
	for (auto creator = suitableCreators.first; creator != suitableCreators.second; ++creator)
	{
		if (!creator->second.matches(opSign))
			continue;
		auto operation = creator->second.creator(opSign.operands);
		if (operation)
			return operation;
	}
	return {};
}

void Operation::add(const std::string& operation, const Operation::Specification& specification)
{
	creators.emplace(operation, specification);
}

class OperationCreator
{
public:
	virtual Operation::Ref create() const = 0;
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

	virtual Operation::Ref create() const override
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
OpCreator<AddOperation> addOp{ "Add" };
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


Operation::Ref Operation::create(OperationType opType)
{
	return OperationCreator::creators[size_t(opType)]->create();
}

Operation::Ref Operation::create(const std::string& opType)
{
	return create(OperationCreator::names.at(opType));
}

const std::string& Operation::getName(OperationType opType)
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


void ValueOperation::execute(Executor& executor) const
{
	executor.set(destinationIndex, value);
}


void CloneValueOperation::execute(Executor& executor) const
{
	executor.set(destinationIndex, value->clone());
}


void LocalValueOperationBase::serialize(Serializer& serializer)
{
	ValueOperationBase::serialize(serializer);
	serializer.serialize(sourceIndex, "sourceIndex");
}


void LocalValueOperation::execute(Executor& executor) const
{
	executor.set(destinationIndex, executor.get(sourceIndex));
}


void CloneLocalValueOperation::execute(Executor& executor) const
{
	executor.set(destinationIndex, executor.get(sourceIndex)->clone());
}


void SwapOperation::execute(Executor& executor) const
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


void AddOperation::execute(Executor& executor) const
{
	*executor.get(destinationIndex) += *executor.get(sourceIndex);
}


void ComparisonOperation::serialize(Serializer& serializer)
{
	serializer.serialize(destinationIndex, "destIndex");
	serializer.serialize(sourceIndex0, "sourceIndex0");
	serializer.serialize(sourceIndex1, "sourceIndex1");
}


void EqualOperation::execute(Executor& executor) const
{
	executor.set(destinationIndex, *executor.get(sourceIndex0) == *executor.get(sourceIndex1) ? TypeBool::trueValue : TypeBool::falseValue);
}

void NotEqualOperation::execute(Executor& executor) const
{
	executor.set(destinationIndex, *executor.get(sourceIndex0) == *executor.get(sourceIndex1) ? TypeBool::falseValue : TypeBool::trueValue);
}

void LessOperation::execute(Executor& executor) const
{
	executor.set(destinationIndex, *executor.get(sourceIndex0) < *executor.get(sourceIndex1) ? TypeBool::trueValue : TypeBool::falseValue);
}

void GreaterOperation::execute(Executor& executor) const
{
	executor.set(destinationIndex, *executor.get(sourceIndex1) < *executor.get(sourceIndex0) ? TypeBool::trueValue : TypeBool::falseValue);
}

void LessEqualOperation::execute(Executor& executor) const
{
	executor.set(destinationIndex, *executor.get(sourceIndex1) < *executor.get(sourceIndex0) ? TypeBool::falseValue : TypeBool::trueValue);
}

void GreaterEqualOperation::execute(Executor& executor) const
{
	executor.set(destinationIndex, *executor.get(sourceIndex0) < *executor.get(sourceIndex1) ? TypeBool::falseValue : TypeBool::trueValue);
}


void JumpOperation::execute(Executor& executor) const
{
	executor.getCode().setNextOperation(nextOperation);
}

void JumpOperation::serialize(Serializer& serializer)
{
	serializer.serialize(nextOperation, "to");
	opType = getType();
}

void JumpIfOperation::execute(Executor& executor) const
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


void IfOperation::execute(Executor& executor) const
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

void IfElseOperation::execute(Executor& executor) const
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