#include <CppScript/Operations.h>
#include <CppScript/Serializer.h>
#include <CppScript/Execution.h>
#include <CppScript/BasicTypes.h>
#include <array>

namespace CppScript
{

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
OpCreator<ReadOperation> readOp{ "Read" };
OpCreator<AssignOperation> assignOp{ "Assign" };
OpCreator<SwapOperation> swapOp{ "Swap" };
OpCreator<CloneOperation> cloneOp{ "Clone" };
OpCreator<AddOperation> addOp{ "Add" };
OpCreator<EqualOperation> equalOp{ "Equal" };
OpCreator<NotEqualOperation> notEqualOp{ "NotEqual" };
OpCreator<LessOperation> lessOp{ "Less" };
OpCreator<GreaterOperation> greaterOp{ "Greater" };
OpCreator<LessEqualOperation> lessEqualOp{ "LessEqual" };
OpCreator<GreaterEqualOperation> greaterEqualOp{ "GreaterEqual" };


Operation::Ref Operation::create(OperationType opType)
{
	return OperationCreator::creators[size_t(opType)]->create();
}

Operation::Ref Operation::create(const std::string& opType)
{
	return create(OperationCreator::names.at(opType));
}

const std::string& getName(OperationType opType)
{
	return OperationCreator::creators[size_t(opType)]->getName();
}


TypeBase::Ref ValueOperation::execute(Executor& executor) const
{
	return value;
}

void ValueOperation::serialize(Serializer& serializer)
{
	serializer.serialize(value);
}


TypeBase::Ref ReadOperation::execute(Executor& executor) const
{
	return executor.getContext().get(variableName);
}

void ReadOperation::serialize(Serializer& serializer)
{
	serializer.serialize(variableName);
}


TypeBase::Ref AssignOperation::execute(Executor& executor) const
{
	return executor.getContext().set(variableName, sourceOperation->execute(executor));
}

void AssignOperation::serialize(Serializer& serializer)
{
	serializer.serialize(variableName);
	serializer.serialize(sourceOperation);
}


TypeBase::Ref SwapOperation::execute(Executor& executor) const
{
	executor.getContext().swap(variableName1, variableName2);
	return {};
}

void SwapOperation::serialize(Serializer& serializer)
{
	serializer.serialize(variableName1);
	serializer.serialize(variableName2);
}


TypeBase::Ref CloneOperation::execute(Executor& executor) const
{
	return sourceOperation->execute(executor)->clone();
}

void CloneOperation::serialize(Serializer& serializer)
{
	serializer.serialize(sourceOperation);
}


void AccumulatorOperation::serialize(Serializer& serializer)
{
	serializer.serialize(destinationOperation);
	serializer.serialize(sourceOperation);
}

TypeBase::Ref AddOperation::execute(Executor& executor) const
{
	return (*destinationOperation->execute(executor)) += *sourceOperation->execute(executor);
}


void ComparisonOperation::serialize(Serializer& serializer)
{
	serializer.serialize(sourceOperation1);
	serializer.serialize(sourceOperation2);
}

TypeBase::Ref EqualOperation::execute(Executor& executor) const
{
	return (*sourceOperation1->execute(executor)) == *sourceOperation2->execute(executor) ? TypeBool::trueValue : TypeBool::falseValue;
}

TypeBase::Ref NotEqualOperation::execute(Executor& executor) const
{
	return (*sourceOperation1->execute(executor)) == *sourceOperation2->execute(executor) ? TypeBool::falseValue : TypeBool::trueValue;
}

TypeBase::Ref LessOperation::execute(Executor& executor) const
{
	return (*sourceOperation1->execute(executor)) < *sourceOperation2->execute(executor) ? TypeBool::trueValue : TypeBool::falseValue;
}

TypeBase::Ref GreaterOperation::execute(Executor& executor) const
{
	return (*sourceOperation2->execute(executor)) < *sourceOperation1->execute(executor) ? TypeBool::trueValue : TypeBool::falseValue;
}

TypeBase::Ref LessEqualOperation::execute(Executor& executor) const
{
	auto value1 = sourceOperation1->execute(executor);
	auto value2 = sourceOperation2->execute(executor);
	return *value1 < *value2 || *value1 == *value2  ? TypeBool::trueValue : TypeBool::falseValue;
}

TypeBase::Ref GreaterEqualOperation::execute(Executor& executor) const
{
	return (*sourceOperation1->execute(executor)) < *sourceOperation2->execute(executor) ? TypeBool::falseValue : TypeBool::trueValue;
}

}