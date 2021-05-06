#pragma once

#include <CppScript/Operations.h>

namespace CppScript
{

class CodeBlock;

class Executor
{
public:
	void run();

	void pushCode(std::unique_ptr<CodeBlock> code);
	std::unique_ptr<CodeBlock> popCode();
	size_t sizeCode() const;

	TypeBase::Ref& get(size_t index);
	void set(size_t index, TypeBase::Ref value);
	/*TypeBase::Ref& top(size_t index = 0);
	TypeBase::Ref& push(TypeBase::Ref value);
	TypeBase::Ref pop();*/

	size_t size() const;
	void resize(size_t size);

private:
	std::vector<std::unique_ptr<CodeBlock>> codeStack;
	std::vector<TypeBase::Ref> dataFrame;
};


enum class BlockType { Sequence, Loop };


class CodeBlock
{
public:
	explicit CodeBlock(const std::vector<Operation::Ref>& code);
	virtual ~CodeBlock() = default;

	virtual const Operation& getOperation();
	virtual BlockType getType() const;

	size_t getBottom() const;
	void setBottom(size_t bottom);

protected:
	const std::vector<Operation::Ref>& operations;
	size_t currentOperation{ 0 };
	size_t dataBottom{ 0 };

	class BlockEndOperation : public Operation
	{
		void execute(Executor& executor) const override;
		void serialize(Serializer& serializer) override {}
	};

	static BlockEndOperation blockEndOperation;
};

}
