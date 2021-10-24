#pragma once

#include <CppScript/Types.h>
#include <memory>
#include <vector>
#include <variant>
#include <functional>
#include <unordered_map>


namespace CppScript
{

	class MemoryPlace
	{
	public:
		enum Type
		{
			Local,
			Module,
			Last
		};

		MemoryPlace() = default;
		MemoryPlace(Type memType, int memIndex) : memoryType(memType), memoryIndex(memIndex)
		{}

		Type memoryType{ Type::Last };
		int memoryIndex{ 0 };
	};

	class Executor;

	class Operation
	{
	public:
		virtual ~Operation() = default;

		virtual void execute(Executor& executor) const = 0;

		using Ref = std::unique_ptr<Operation>;

		struct Argument
		{
			const TypeIdBase* typeId{ nullptr };
			MemoryPlace place;
		};

		using Arguments = std::vector<Argument>;

		struct Call
		{
			std::string operation;
			Arguments arguments;
		};

		static Ref create(const Call& opCall);

		class Specification
		{
		public:
			struct ArgumentType
			{
				const TypeIdBase* typeId{ nullptr };
				size_t minCount{ 1 };
				size_t maxCount{ 1 };
			};

			using ArgumentTypes = std::vector<ArgumentType>;
			using Creator = std::function<Ref(const Arguments&)>;

			Specification(std::string&& operation, ArgumentTypes&& arguments, Creator&& creator);
			~Specification();

			static Ref createOperation(const Call& opCall);

			bool matches(const Call& opCall) const;

		private:
			ArgumentTypes arguments;
			Creator creator;

			using Map = std::unordered_multimap<std::string, const Specification*>;

			Map::const_iterator specPosition{ specifications.cend() };

			static Map specifications;
		};
	};

	class OperationModule;

	class OperationEntry
	{
	public:
		size_t entry;
		size_t localFrame;
		OperationModule* owner{ nullptr };
	};

	class OperationFunction : public OperationEntry
	{
	public:
		std::string id;
		Operation::Specification specification;
	};

	class OperationModule
	{
	public:
		std::vector<TypeBase::Ref> data;
		std::vector<Operation::Ref> operations;
		std::vector<OperationFunction> functions;
		std::optional<OperationEntry> initialization;
	};


	//deprecated

	class ExecutorOld;
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

	enum class OperandSourceOld
	{
		Invalid = 0,
		DirectValue = 1,
		MemoryValue = 2
	};

	OperandSourceOld operator&(OperandSourceOld first, OperandSourceOld second);
	OperandSourceOld operator|(OperandSourceOld first, OperandSourceOld second);


	class OperationOld
	{
	public:
		virtual ~OperationOld() = default;

		virtual void execute(ExecutorOld& executor) const = 0;

		virtual void serialize(Serializer& serializer) {}

		using Ref = std::unique_ptr<OperationOld>;

		static Ref create(OperationType opType);
		static Ref create(const std::string& opType);
		static const std::string& getName(OperationType opType);

		OperationType opType;
		static OperationType getOperationType(const std::string& opName);
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


	class ValueOperationBase : public OperationOld
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
		void execute(ExecutorOld& executor) const override;
		void serialize(Serializer& serializer) override
		{
			DirectValueOperationBase::serialize(serializer);
			opType = getType();
		}
	};

	class CloneValueOperation : public DirectValueOperationBase, public OperationTypeBase<OperationType::CloneValue>
	{
	public:
		void execute(ExecutorOld& executor) const override;
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
		void execute(ExecutorOld& executor) const override;
		void serialize(Serializer& serializer) override
		{
			LocalValueOperationBase::serialize(serializer);
			opType = getType();
		}
	};

	class CloneLocalValueOperation : public LocalValueOperationBase, public OperationTypeBase<OperationType::CloneLocalValue>
	{
	public:
		void execute(ExecutorOld& executor) const override;
		void serialize(Serializer& serializer) override
		{
			LocalValueOperationBase::serialize(serializer);
			opType = getType();
		}
	};


	class SwapOperation : public OperationOld, public OperationTypeBase<OperationType::Swap>
	{
	public:
		void execute(ExecutorOld& executor) const override;
		void serialize(Serializer& serializer) override;

	//protected:
		IntValue index0{ 0 };
		IntValue index1{ 0 };
	};


	class AccumulationOperation : public OperationOld
	{
	public:
		void serialize(Serializer& serializer) override;

	//protected:
		IntValue destinationIndex{ 0 };
		IntValue sourceIndex{ 0 };
	};


	class AddOperationOld : public AccumulationOperation, public OperationTypeBase<OperationType::Add>
	{
	protected:
		void execute(ExecutorOld& executor) const override;
		void serialize(Serializer& serializer) override
		{
			AccumulationOperation::serialize(serializer);
			opType = getType();
		}
	};


	class ComparisonOperation : public OperationOld
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
		void execute(ExecutorOld& executor) const override;
		void serialize(Serializer& serializer) override
		{
			ComparisonOperation::serialize(serializer);
			opType = getType();
		}
	};

	class NotEqualOperation : public ComparisonOperation, public OperationTypeBase<OperationType::NotEqual>
	{
	public:
		void execute(ExecutorOld& executor) const override;
		void serialize(Serializer& serializer) override
		{
			ComparisonOperation::serialize(serializer);
			opType = getType();
		}
	};

	class LessOperation : public ComparisonOperation, public OperationTypeBase<OperationType::Less>
	{
	public:
		void execute(ExecutorOld& executor) const override;
		void serialize(Serializer& serializer) override
		{
			ComparisonOperation::serialize(serializer);
			opType = getType();
		}
	};

	class GreaterOperation : public ComparisonOperation, public OperationTypeBase<OperationType::Greater>
	{
	public:
		void execute(ExecutorOld& executor) const override;
		void serialize(Serializer& serializer) override
		{
			ComparisonOperation::serialize(serializer);
			opType = getType();
		}
	};

	class LessEqualOperation : public ComparisonOperation, public OperationTypeBase<OperationType::LessEqual>
	{
	public:
		void execute(ExecutorOld& executor) const override;
		void serialize(Serializer& serializer) override
		{
			ComparisonOperation::serialize(serializer);
			opType = getType();
		}
	};

	class GreaterEqualOperation : public ComparisonOperation, public OperationTypeBase<OperationType::GreaterEqual>
	{
	public:
		void execute(ExecutorOld& executor) const override;
		void serialize(Serializer& serializer) override
		{
			ComparisonOperation::serialize(serializer);
			opType = getType();
		}
	};


	class JumpOperation : public OperationOld, public OperationTypeBase<OperationType::Jump>
	{
	public:
		void execute(ExecutorOld& executor) const override;
		void serialize(Serializer& serializer) override;

	//protected:
		IntValue nextOperation{ 0 };
	};

	class JumpIfOperation : public OperationOld, public OperationTypeBase<OperationType::JumpIf>
	{
	public:
		void execute(ExecutorOld& executor) const override;
		void serialize(Serializer& serializer) override;

	//protected:
		IntValue index{ 0 };
		IntValue nextOperation{ 0 };
	};


	class IfOperation : public OperationOld, public OperationTypeBase<OperationType::If>
	{
	public:
		void execute(ExecutorOld& executor) const override;
		void serialize(Serializer& serializer) override;

	private:
		IntValue index{ 0 };
		std::vector<OperationOld::Ref> operations;
	};

	class IfElseOperation : public OperationOld, public OperationTypeBase<OperationType::IfElse>
	{
	public:
		void execute(ExecutorOld& executor) const override;
		void serialize(Serializer& serializer) override;

	private:
		IntValue index{ 0 };
		std::vector<OperationOld::Ref> trueOperations;
		std::vector<OperationOld::Ref> falseOperations;
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