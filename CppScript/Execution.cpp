#include <CppScript/Execution.h>


namespace CppScript
{

void Executor::run()
{
	while (currentCode != nullptr)
		currentCode->getOperation().execute(*this);
}

void Executor::runIt()
{
	while (currentCode != nullptr)
		currentCode->getOperationIt().execute(*this);
}

void Executor::runFib()
{
	currentCode->operations[0]->execute(*this);
	currentCode->operations[1]->execute(*this);
	currentCode->operations[2]->execute(*this);
	currentCode->operations[3]->execute(*this);
	do
	{
		currentCode->operations[4]->execute(*this);
		currentCode->operations[5]->execute(*this);
		currentCode->operations[6]->execute(*this);
		currentCode->operations[7]->execute(*this);
	} 	while (get(5)->as<TypeBool::ValueType>());
}

void Executor::runFibNC()
{
	const auto& set0 = static_cast<const CloneValueOperation&>(*currentCode->operations[0]);
	set(set0.destinationIndex, set0.value->clone());
	const auto& set1 = static_cast<const CloneValueOperation&>(*currentCode->operations[1]);
	set(set1.destinationIndex, set1.value->clone());
	const auto& set2 = static_cast<const CloneValueOperation&>(*currentCode->operations[2]);
	set(set2.destinationIndex, set2.value->clone());
	const auto& set3 = static_cast<const ValueOperation&>(*currentCode->operations[3]);
	set(set3.destinationIndex, set3.value);
	do
	{
		const auto& add0 = static_cast<const AddOperation&>(*currentCode->operations[4]);
		*get(add0.destinationIndex) += *get(add0.sourceIndex);
		const auto& swap0 = static_cast<const SwapOperation&>(*currentCode->operations[5]);
		std::swap(get(swap0.index0), get(swap0.index1));
		const auto& add1 = static_cast<const AddOperation&>(*currentCode->operations[6]);
		*get(add1.destinationIndex) += *get(add1.sourceIndex);
		const auto& grt0 = static_cast<const GreaterOperation&>(*currentCode->operations[7]);
		set(grt0.destinationIndex, *get(grt0.sourceIndex1) < *get(grt0.sourceIndex0) ? TypeBool::trueValue : TypeBool::falseValue);
	} 	while (get(5)->as<TypeBool::ValueType>());
}

void Executor::runFibNOP()
{
	set(1, TypeInt::create(0));
	set(2, TypeInt::create(1));
	set(3, TypeInt::create(1));
	set(4, TypeInt::create(1));
	do
	{
		*get(1) += *get(2);
		std::swap(get(1), get(2));
		*get(3) += *get(4);
		set(5, *get(3) < *get(0) ? TypeBool::trueValue : TypeBool::falseValue);
	} 	while (get(5)->as<TypeBool::ValueType>());
}

void Executor::runFibNOP2()
{
	auto v1 = TypeInt::create(0);
	auto v2 = TypeInt::create(1);
	auto v3 = TypeInt::create(1);
	auto v4 = TypeInt::create(1);
	auto v5 = TypeBool::trueValue;
	do
	{
		*v1 += *v2;
		std::swap(v1, v2);
		*v3 += *v4;
		v5 = *v3 < *get(0) ? TypeBool::trueValue : TypeBool::falseValue;
	} 	while (v5->as<TypeBool::ValueType>());
	set(2, v2);
}

void Executor::runFibNOP3()
{
	TypeInt::ValueType v1 = 0;
	TypeInt::ValueType v2 = 1;
	TypeInt::ValueType v3 = 1;
	TypeInt::ValueType v4 = 1;
	bool v5 = true;
	do
	{
		v1 += v2;
		std::swap(v1, v2);
		v3 += v4;
		v5 = v3 < get(0)->as<TypeInt::ValueType>();
	} while (v5);
	set(2, TypeInt::create(v2));
}

void Executor::pushCode(const std::vector<Operation::Ref>& code)
{
	codeStack.emplace_back(code, size());
	currentCode = &codeStack.back();
}

void Executor::popCode()
{
	codeStack.pop_back();
	currentCode = codeStack.empty() ? nullptr : &codeStack.back();
}

CodeBlock& Executor::getCode()
{
	return *currentCode;
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

size_t Executor::size() const
{
	return dataFrame.size();
}

void Executor::resize(size_t size)
{
	dataFrame.resize(size);
}


CodeBlock::CodeBlock(const std::vector<Operation::Ref>& code, size_t bottom)
	: operations(code), currentOperation(operations.begin()), dataBottom(bottom)
{}

const Operation& CodeBlock::getOperation()
{
	return *operations[currentOperationIdx++];
}

const Operation& CodeBlock::getOperationIt()
{
	return **currentOperation++;
}

void CodeBlock::setNextOperation(size_t index)
{
	_ASSERT(index < operations.size());
	currentOperationIdx = index;
	currentOperation = std::next(operations.begin(), index);
}

size_t CodeBlock::getBottom() const
{
	return dataBottom;
}

void CodeBlock::setBottom(size_t bottom)
{
	dataBottom = bottom;
}


}