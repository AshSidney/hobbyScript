#pragma once

#include <CppScript/Types.h>

#include <CppScript/Base.h>
#include <CppScript/TypeWrapper.h>

namespace CppScript
{

	class Executor;
	class Serializer;

	enum class OperationType
	{
		Value,
		Read,
		Clone,
		Assign,
		Add,
		Last
	};

	class Operation
	{
	public:
		virtual ~Operation() = default;

		virtual TypeBase::Ref execute(Executor& executor) const = 0;
		virtual void serialize(Serializer& serializer) = 0;

		using Ref = std::unique_ptr<Operation>;
		static Ref create(OperationType opType);
		static Ref create(const std::string& opType);
		static const std::string& getName(OperationType opType);
	};

	template <OperationType T> class OperationTypeBase
	{
	public:
		virtual OperationType getType() const
		{
			return T;
		}

		static constexpr OperationType getOpType()
		{
			return T;
		}
	};


	class ValueOperation : public Operation, public OperationTypeBase<OperationType::Value>
	{
	public:
		virtual TypeBase::Ref execute(Executor& executor) const override;
		virtual void serialize(Serializer& serializer) override;

	private:
		TypeBase::Ref value;
	};


	class ReadOperation : public Operation, public OperationTypeBase<OperationType::Read>
	{
	public:
		virtual TypeBase::Ref execute(Executor& executor) const override;
		virtual void serialize(Serializer& serializer) override;

	private:
		std::string variableName;
	};


	class AssignOperation : public Operation, public OperationTypeBase<OperationType::Assign>
	{
	public:
		virtual TypeBase::Ref execute(Executor& executor) const override;
		virtual void serialize(Serializer& serializer) override;

	private:
		std::string variableName;
		Operation::Ref sourceOperation;
	};


	class CloneOperation : public Operation, public OperationTypeBase<OperationType::Clone>
	{
	public:
		virtual TypeBase::Ref execute(Executor& executor) const override;
		virtual void serialize(Serializer& serializer) override;

	private:
		Operation::Ref sourceOperation;
	};


	class AddOperation : public Operation, public OperationTypeBase<OperationType::Add>
	{
	public:
		virtual TypeBase::Ref execute(Executor& executor) const override;
		virtual void serialize(Serializer& serializer) override;

	private:
		Operation::Ref destinationOperation;
		Operation::Ref sourceOperation;
	};


	class Context;

	class OperationOld : public Visitable<OperationOld, Element, ElementVisitor>
	{
	public:
		virtual void setUp(const Elements& data) = 0;
		virtual TypeWrapperBase::Ref execute(Context& context) const = 0;
	};

	class Sum : public Visitable<Sum, OperationOld, ElementVisitor>
	{
	public:
		void setUp(const Elements& data) override;
		TypeWrapperBase::Ref execute(Context& context) const override;

		//void accept(VisitorOld& visitor) const override;
		//void accept(VisitorOld& visitor) override;

	private:
		Elements operands;
	};

	class Iterator : public OperationOld
	{

	};

	/*class RangeGenerator : public OperationOld
	{
	public:
		//virtual Ref execute() override;

	protected:
		//IntValue currentValue;
	};*/

}