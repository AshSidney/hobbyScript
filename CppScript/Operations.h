#pragma once

#include <CppScript/Types.h>


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
		Swap,
		Add,
		Equal,
		NotEqual,
		Less,
		Greater,
		LessEqual,
		GreaterEqual,
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


	class SwapOperation : public Operation, public OperationTypeBase<OperationType::Swap>
	{
	public:
		virtual TypeBase::Ref execute(Executor& executor) const override;
		virtual void serialize(Serializer& serializer) override;

	private:
		std::string variableName1;
		std::string variableName2;
	};


	class CloneOperation : public Operation, public OperationTypeBase<OperationType::Clone>
	{
	public:
		virtual TypeBase::Ref execute(Executor& executor) const override;
		virtual void serialize(Serializer& serializer) override;

	private:
		Operation::Ref sourceOperation;
	};


	class AccumulatorOperation : public Operation
	{
	public:
		virtual void serialize(Serializer& serializer) override;

	protected:
		Operation::Ref destinationOperation;
		Operation::Ref sourceOperation;
	};


	class AddOperation : public AccumulatorOperation, public OperationTypeBase<OperationType::Add>
	{
	public:
		virtual TypeBase::Ref execute(Executor& executor) const override;
	};


	class ComparisonOperation : public Operation
	{
	public:
		virtual void serialize(Serializer& serializer) override;

	protected:
		Operation::Ref sourceOperation1;
		Operation::Ref sourceOperation2;
	};


	class EqualOperation : public ComparisonOperation, public OperationTypeBase<OperationType::Equal>
	{
	public:
		virtual TypeBase::Ref execute(Executor& executor) const override;
	};

	class NotEqualOperation : public ComparisonOperation, public OperationTypeBase<OperationType::NotEqual>
	{
	public:
		virtual TypeBase::Ref execute(Executor& executor) const override;
	};

	class LessOperation : public ComparisonOperation, public OperationTypeBase<OperationType::Less>
	{
	public:
		virtual TypeBase::Ref execute(Executor& executor) const override;
	};

	class GreaterOperation : public ComparisonOperation, public OperationTypeBase<OperationType::Greater>
	{
	public:
		virtual TypeBase::Ref execute(Executor& executor) const override;
	};

	class LessEqualOperation : public ComparisonOperation, public OperationTypeBase<OperationType::LessEqual>
	{
	public:
		virtual TypeBase::Ref execute(Executor& executor) const override;
	};

	class GreaterEqualOperation : public ComparisonOperation, public OperationTypeBase<OperationType::GreaterEqual>
	{
	public:
		virtual TypeBase::Ref execute(Executor& executor) const override;
	};
}