#include <CppScript/Execution.h>


namespace CppScript
{

void Executor::run()
{
	while (sizeCode() > 0)
		codeStack.back()->getOperation().execute(*this);
}

void Executor::pushCode(std::unique_ptr<CodeBlock> code)
{
	codeStack.push_back(std::move(code));
	codeStack.back()->setBottom(size());
}

std::unique_ptr<CodeBlock> Executor::popCode()
{
	auto popped = std::move(codeStack.back());
	codeStack.pop_back();
	return popped;
}

size_t Executor::sizeCode() const
{
	return codeStack.size();
}

TypeBase::Ref& Executor::get(size_t index)
{
	_ASSERT(index < size());
	return dataFrame[index];
}

void Executor::set(size_t index, TypeBase::Ref value)
{
	_ASSERT(index < size());
	dataFrame[index] = value;
}

/*TypeBase::Ref& Executor::top(size_t index)
{
	_ASSERT(index < size());
	return *std::next(dataStack.rbegin(), index);
}

TypeBase::Ref& Executor::push(TypeBase::Ref value)
{
	dataStack.push_back(value);
	return dataStack.back();
}

TypeBase::Ref Executor::pop()
{
	auto result = dataStack.back();
	dataStack.pop_back();
	return result;
}*/

size_t Executor::size() const
{
	return dataFrame.size();
}

void Executor::resize(size_t size)
{
	dataFrame.resize(size);
}


CodeBlock::CodeBlock(const std::vector<Operation::Ref>& code) : operations(code)
{}

const Operation& CodeBlock::getOperation()
{
	if (currentOperation < operations.size())
		return *operations[currentOperation++];
	return blockEndOperation;
}

BlockType CodeBlock::getType() const
{
	return BlockType::Sequence;
}

size_t CodeBlock::getBottom() const
{
	return dataBottom;
}

void CodeBlock::setBottom(size_t bottom)
{
	dataBottom = bottom;
}

void CodeBlock::BlockEndOperation::execute(Executor& executor) const
{
	auto codeBlock = executor.popCode();
	if (executor.sizeCode() > 0)
		executor.resize(codeBlock->getBottom());
}

CodeBlock::BlockEndOperation CodeBlock::blockEndOperation;


}