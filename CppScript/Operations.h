#pragma once

#include <CppScript/Types.h>
#include <memory>
#include <vector>
#include <variant>
#include <functional>
#include <unordered_map>


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

		Jump,
		JumpIf,

		If,
		IfElse,
		Loop,
		Break,

		BlockEnd,
		Last
	};

	enum class OperandSource
	{
		Invalid = 0,
		DirectValue = 1,
		MemoryValue = 2
	};

	OperandSource operator&(OperandSource first, OperandSource second);
	OperandSource operator|(OperandSource first, OperandSource second);


	class Operation
	{
	public:
		virtual ~Operation() = default;

		virtual void execute(Executor& executor) const = 0;

		virtual void serialize(Serializer& serializer) = 0;

		using Ref = std::unique_ptr<Operation>;

		struct OperandType
		{
			const TypeIdBase& typeId;
			OperandSource source{ OperandSource::Invalid };
		};

		struct Signature
		{
			std::string operation;
			std::vector<OperandType> operands;
		};

		static Ref create(const Signature& opSign);


		struct Specification
		{
			struct VariadicTypeOperand
			{
				const TypeIdBase* typeId;
				OperandSource source{ OperandSource::Invalid };
				size_t minCount{ 1 };
				size_t maxCount{ 1 };
			};
			std::vector<VariadicTypeOperand> operands;

			std::function<Ref(const std::vector<OperandType>&)> creator;

			bool matches(const Signature& opSign) const;
		};

		static void add(const std::string& operation, const Specification& specification);


		/*struct TypePlace
		{
			const TypeIdBase& typeId;
			size_t index;
		};

		virtual std::vector<TypePlace> getChangedTypes() const
		{
			return {};
		}*/

		//deprecated
		static Ref create(OperationType opType);
		static Ref create(const std::string& opType);
		static const std::string& getName(OperationType opType);

		OperationType opType;
		static OperationType getOperationType(const std::string& opName);

	private:
		static std::unordered_multimap<std::string, Specification> creators;
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

	//protected:
		IntValue destinationIndex{ 0 };
	};

	class DirectValueOperationBase : public ValueOperationBase
	{
	public:
		void serialize(Serializer& serializer) override;

	//protected:
		TypeBase::Ref value;
	};

	class ValueOperation : public DirectValueOperationBase, public OperationTypeBase<OperationType::Value>
	{
	public:
		void execute(Executor& executor) const override;
		void serialize(Serializer& serializer) override
		{
			DirectValueOperationBase::serialize(serializer);
			opType = getType();
		}
	};

	class CloneValueOperation : public DirectValueOperationBase, public OperationTypeBase<OperationType::CloneValue>
	{
	public:
		void execute(Executor& executor) const override;
		void serialize(Serializer& serializer) override
		{
			DirectValueOperationBase::serialize(serializer);
			opType = getType();
		}
	};


	class LocalValueOperationBase : public ValueOperationBase
	{
	public:
		void serialize(Serializer& serializer) override;

	//protected:
		IntValue sourceIndex{ 0 };
	};

	class LocalValueOperation : public LocalValueOperationBase, public OperationTypeBase<OperationType::LocalValue>
	{
	public:
		void execute(Executor& executor) const override;
		void serialize(Serializer& serializer) override
		{
			LocalValueOperationBase::serialize(serializer);
			opType = getType();
		}
	};

	class CloneLocalValueOperation : public LocalValueOperationBase, public OperationTypeBase<OperationType::CloneLocalValue>
	{
	public:
		void execute(Executor& executor) const override;
		void serialize(Serializer& serializer) override
		{
			LocalValueOperationBase::serialize(serializer);
			opType = getType();
		}
	};


	class SwapOperation : public Operation, public OperationTypeBase<OperationType::Swap>
	{
	public:
		void execute(Executor& executor) const override;
		void serialize(Serializer& serializer) override;

	//protected:
		IntValue index0{ 0 };
		IntValue index1{ 0 };
	};


	class AccumulationOperation : public Operation
	{
	public:
		void serialize(Serializer& serializer) override;

	//protected:
		IntValue destinationIndex{ 0 };
		IntValue sourceIndex{ 0 };
	};


	class AddOperation : public AccumulationOperation, public OperationTypeBase<OperationType::Add>
	{
	protected:
		void execute(Executor& executor) const override;
		void serialize(Serializer& serializer) override
		{
			AccumulationOperation::serialize(serializer);
			opType = getType();
		}
	};


	class ComparisonOperation : public Operation
	{
	public:
		virtual void serialize(Serializer& serializer) override;

	//protected:
		IntValue destinationIndex{ 0 };
		IntValue sourceIndex0{ 0 };
		IntValue sourceIndex1{ 0 };
	};


	class EqualOperation : public ComparisonOperation, public OperationTypeBase<OperationType::Equal>
	{
	public:
		void execute(Executor& executor) const override;
		void serialize(Serializer& serializer) override
		{
			ComparisonOperation::serialize(serializer);
			opType = getType();
		}
	};

	class NotEqualOperation : public ComparisonOperation, public OperationTypeBase<OperationType::NotEqual>
	{
	public:
		void execute(Executor& executor) const override;
		void serialize(Serializer& serializer) override
		{
			ComparisonOperation::serialize(serializer);
			opType = getType();
		}
	};

	class LessOperation : public ComparisonOperation, public OperationTypeBase<OperationType::Less>
	{
	public:
		void execute(Executor& executor) const override;
		void serialize(Serializer& serializer) override
		{
			ComparisonOperation::serialize(serializer);
			opType = getType();
		}
	};

	class GreaterOperation : public ComparisonOperation, public OperationTypeBase<OperationType::Greater>
	{
	public:
		void execute(Executor& executor) const override;
		void serialize(Serializer& serializer) override
		{
			ComparisonOperation::serialize(serializer);
			opType = getType();
		}
	};

	class LessEqualOperation : public ComparisonOperation, public OperationTypeBase<OperationType::LessEqual>
	{
	public:
		void execute(Executor& executor) const override;
		void serialize(Serializer& serializer) override
		{
			ComparisonOperation::serialize(serializer);
			opType = getType();
		}
	};

	class GreaterEqualOperation : public ComparisonOperation, public OperationTypeBase<OperationType::GreaterEqual>
	{
	public:
		void execute(Executor& executor) const override;
		void serialize(Serializer& serializer) override
		{
			ComparisonOperation::serialize(serializer);
			opType = getType();
		}
	};


	class JumpOperation : public Operation, public OperationTypeBase<OperationType::Jump>
	{
	public:
		void execute(Executor& executor) const override;
		void serialize(Serializer& serializer) override;

	//protected:
		IntValue nextOperation{ 0 };
	};

	class JumpIfOperation : public Operation, public OperationTypeBase<OperationType::JumpIf>
	{
	public:
		void execute(Executor& executor) const override;
		void serialize(Serializer& serializer) override;

	//protected:
		IntValue index{ 0 };
		IntValue nextOperation{ 0 };
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


	/*class LoopOperation : public Operation, public OperationTypeBase<OperationType::Loop>
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
	};*/

}