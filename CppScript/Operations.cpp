#include <CppScript/Operations.h>
#include <CppScript/TypeWrapper.h>
#include <CppScript/Serializer.h>
#include <CppScript/Execution.h>
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

OperationCreator::Creators OperationCreator::creators;
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
OpCreator<CloneOperation> cloneOp{ "Clone" };
OpCreator<AddOperation> addOp{ "Add" };


Operation::Ref Operation::create(OperationType opType)
{
	return OperationCreator::creators[size_t(opType)]->create();
}

Operation::Ref Operation::create(const std::string& opType)
{
	return create(OperationCreator::names[opType]);
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


TypeBase::Ref CloneOperation::execute(Executor& executor) const
{
	return sourceOperation->execute(executor)->clone();
}

void CloneOperation::serialize(Serializer& serializer)
{
	serializer.serialize(sourceOperation);
}


TypeBase::Ref AddOperation::execute(Executor& executor) const
{
	return (*destinationOperation->execute(executor)) += *sourceOperation->execute(executor);
}

void AddOperation::serialize(Serializer& serializer)
{
	serializer.serialize(destinationOperation);
	serializer.serialize(sourceOperation);
}


/*class SumVisitor : public ElementVisitorFailing
{
public:
	explicit SumVisitor(Context& context) : context(context)
	{}

	void visit(const TypeWrapperBase& wrapper) override
	{
		sumValue += TypeWrapper<long long>::typeDescriptor.get(wrapper);
	}
	void visit(TypeWrapperBase& wrapper) override
	{
		const TypeWrapperBase& constWrapper{ wrapper };
		visit(constWrapper);
	}
	void visit(const Elements& elements) override
	{
		for (const auto& element : elements.elements)
			element->accept(*this);
	}
	void visit(Elements& elements) override
	{
		const Elements& constElements{ elements };
		visit(constElements);
	}
	void visit(const OperationOld& operation) override
	{
		visit(*operation.execute(context));
	}
	void visit(OperationOld& operation) override
	{
		const OperationOld& constOperation{ operation };
		visit(constOperation);
	}

	long long sumValue{ 0 };

protected:
	Context& context;
};

void Sum::setUp(const Elements& data)
{
	operands = data;
}

TypeWrapperBase::Ref Sum::execute(Context& context) const
{
	SumVisitor sum{ context };
	operands.accept(sum);
	return Element::create<TypeWrapper<long long>>(sum.sumValue);
}*/

/*void Sum::accept(VisitorOld& visitor) const
{
	visitor.visit(*this);
}

void Sum::accept(VisitorOld& visitor)
{
	visitor.visit(*this);
}*/


/*Element::Ref RangeGenerator::execute()
{
	return{};
}*/

}