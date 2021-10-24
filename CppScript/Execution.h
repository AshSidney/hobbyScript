#pragma once

#include <CppScript/Operations.h>
#include <CppScript/BasicTypes.h>
#include <array>
#include <chrono>

namespace CppScript
{

	class Executor
	{
	public:
		void run(const Operation& operation);

		void add(OperationModule&& opModule);

		void push(size_t size);
		void pop();

		TypeBase::Ref& get(const MemoryPlace& place);
		void set(const MemoryPlace& place, TypeBase::Ref value);
		void swap(const MemoryPlace& place0, const MemoryPlace& place1);

	private:
		void set(OperationModule* opModule);

		std::vector<OperationModule> modules;

		OperationModule* currentModule{ nullptr };

		std::array<TypeBase::Ref*, MemoryPlace::Type::Last> dataMemories;

		std::vector<TypeBase::Ref> localMemory;

		struct LocalFrame
		{
			size_t previousSize{ 0 };
			OperationModule* opModule{ nullptr };
		};

		std::vector<LocalFrame> localStack{ {} };
	};

	// deprecated

	class CodeBlock;

	class ExecutorOld
	{
	public:
		void run();
		void runIt();
		void runFib();
		void runFibNC();
		void runFibNOP();
		void runFibNOP2();
		void runFibNOP3();

		void pushCode(const std::vector<OperationOld::Ref>& code);
		void popCode();
		CodeBlock& getCode();
		//size_t sizeCode() const;

		TypeBase::Ref& get(size_t index);
		void set(size_t index, TypeBase::Ref value);

		size_t size() const;
		void resize(size_t size);

	private:
		CodeBlock* currentCode{ nullptr };
		std::vector<CodeBlock> codeStack;
		std::vector<TypeBase::Ref> dataFrame;
	};


	class CodeBlock
	{
	public:
		CodeBlock(const std::vector<OperationOld::Ref>& code, size_t bottom);

		const OperationOld& getOperation();
		const OperationOld& getOperationIt();

		void setNextOperation(size_t index);

		size_t getBottom() const;
		void setBottom(size_t bottom);

		class BlockEndOperation : public OperationOld
		{
		public:
			void execute(ExecutorOld& executor) const override
			{
				executor.popCode();
			}
			void serialize(Serializer& serializer) override
			{}
		};

	//protected:
		const std::vector<OperationOld::Ref>& operations;
		std::vector<OperationOld::Ref>::const_iterator currentOperation;
		size_t currentOperationIdx{ 0 };
		size_t dataBottom{ 0 };
	};

}
