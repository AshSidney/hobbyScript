#pragma once

#include <CppScript/Types.h>
#include <vector>


namespace CppScript
{

	class Executor;
	class Serializer;

	enum class OperationType
	{
		Value,
		CloneValue,
		LocalValue,
		CloneLocalValue,

		Swap,

		Add,

		Equal,
		NotEqual,
		Less,
		Greater,
		LessEqual,
		GreaterEqual,

		If,
		IfElse,
		Loop,
		Break,

		Last
	};

	class Operation
	{
	public:
		virtual ~Operation() = default;

		virtual void execute(Executor& executor) const = 0;
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


	class ValueOperationBase : public Operation
	{
	public:
		void serialize(Serializer& serializer) override;

	protected:
		IntValue destinationIndex{ 0 };
	};

	class DirectValueOperationBase : public ValueOperationBase
	{
	public:
		void serialize(Serializer& serializer) override;

	protected:
		TypeBase::Ref value;
	};

	class ValueOperation : public DirectValueOperationBase, public OperationTypeBase<OperationType::Value>
	{
	public:
		void execute(Executor& executor) const override;
	};

	class CloneValueOperation : public DirectValueOperationBase, public OperationTypeBase<OperationType::CloneValue>
	{
	public:
		void execute(Executor& executor) const override;
	};


	class LocalValueOperationBase : public ValueOperationBase
	{
	public:
		void serialize(Serializer& serializer) override;

	protected:
		IntValue sourceIndex{ 0 };
	};

	class LocalValueOperation : public LocalValueOperationBase, public OperationTypeBase<OperationType::LocalValue>
	{
	public:
		void execute(Executor& executor) const override;
	};

	class CloneLocalValueOperation : public LocalValueOperationBase, public OperationTypeBase<OperationType::CloneLocalValue>
	{
	public:
		void execute(Executor& executor) const override;
	};


	class SwapOperation : public Operation, public OperationTypeBase<OperationType::Swap>
	{
	public:
		void execute(Executor& executor) const override;
		void serialize(Serializer& serializer) override;

	protected:
		IntValue index0{ 0 };
		IntValue index1{ 0 };
	};


	class AccumulationOperation : public Operation
	{
	public:
		void serialize(Serializer& serializer) override;

	protected:
		IntValue destinationIndex{ 0 };
		IntValue sourceIndex{ 0 };
	};


	class AddOperation : public AccumulationOperation, public OperationTypeBase<OperationType::Add>
	{
	protected:
		void execute(Executor& executor) const override;
	};


	class ComparisonOperation : public Operation
	{
	public:
		virtual void serialize(Serializer& serializer) override;

	protected:
		IntValue destinationIndex{ 0 };
		IntValue sourceIndex0{ 0 };
		IntValue sourceIndex1{ 0 };
	};


	class EqualOperation : public ComparisonOperation, public OperationTypeBase<OperationType::Equal>
	{
	protected:
		void execute(Executor& executor) const override;
	};

	class NotEqualOperation : public ComparisonOperation, public OperationTypeBase<OperationType::NotEqual>
	{
	protected:
		void execute(Executor& executor) const override;
	};

	class LessOperation : public ComparisonOperation, public OperationTypeBase<OperationType::Less>
	{
	protected:
		void execute(Executor& executor) const override;
	};

	class GreaterOperation : public ComparisonOperation, public OperationTypeBase<OperationType::Greater>
	{
	protected:
		void execute(Executor& executor) const override;
	};

	class LessEqualOperation : public ComparisonOperation, public OperationTypeBase<OperationType::LessEqual>
	{
	protected:
		void execute(Executor& executor) const override;
	};

	class GreaterEqualOperation : public ComparisonOperation, public OperationTypeBase<OperationType::GreaterEqual>
	{
	protected:
		void execute(Executor& executor) const override;
	};


	class IfOperation : public Operation, public OperationTypeBase<OperationType::If>
	{
	public:
		void execute(Executor& executor) const override;
		void serialize(Serializer& serializer) override;

	private:
		IntValue index{ 0 };
		std::vector<Operation::Ref> operations;
	};

	class IfElseOperation : public Operation, public OperationTypeBase<OperationType::IfElse>
	{
	public:
		void execute(Executor& executor) const override;
		void serialize(Serializer& serializer) override;

	private:
		IntValue index{ 0 };
		std::vector<Operation::Ref> trueOperations;
		std::vector<Operation::Ref> falseOperations;
	};


	class LoopOperation : public Operation, public OperationTypeBase<OperationType::Loop>
	{
	public:
		void execute(Executor& executor) const override;
		void serialize(Serializer& serializer) override;

	private:
		std::vector<Operation::Ref> operations;
	};


	class BreakOperation : public Operation, public OperationTypeBase<OperationType::Break>
	{
	public:
		virtual void execute(Executor& executor) const override;
		virtual void serialize(Serializer& serializer) override {}
	};

}