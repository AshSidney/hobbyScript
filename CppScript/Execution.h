#pragma once

#include <CppScript/Operations.h>
#include <CppScript/BasicTypes.h>
#include <chrono>

namespace CppScript
{

class CodeBlock;

class Executor
{
public:
	void run();
	void runIt();
	void runFib();
	void runFibNC();
	void runFibNOP();
	void runFibNOP2();
	void runFibNOP3();

	void pushCode(const std::vector<Operation::Ref>& code);
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
	CodeBlock(const std::vector<Operation::Ref>& code, size_t bottom);

	const Operation& getOperation();
	const Operation& getOperationIt();

	void setNextOperation(size_t index);

	size_t getBottom() const;
	void setBottom(size_t bottom);

	class BlockEndOperation : public Operation
	{
	public:
		void execute(Executor& executor) const override
		{
			executor.popCode();
		}
		void serialize(Serializer& serializer) override
		{}
	};

//protected:
	const std::vector<Operation::Ref>& operations;
	std::vector<Operation::Ref>::const_iterator currentOperation;
	size_t currentOperationIdx{ 0 };
	size_t dataBottom{ 0 };
};

}
