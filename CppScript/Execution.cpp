#include <CppScript/Execution.h>


namespace CppScript
{

void Executor::run(const Operation& operation)
{
	operation.execute(*this);
}

void Executor::add(OperationModule&& opModule)
{
	modules.push_back(std::move(opModule));
	set(&modules.back());
}

void Executor::push(size_t size)
{
	const auto currentSize = localMemory.size();
	localStack.push_back({ currentSize, currentModule });
	localMemory.resize(currentSize + size);
	dataMemories[MemoryPlace::Type::Local] = localMemory.data() + currentSize;
}

void Executor::pop()
{
	set(localStack.back().opModule);
	localMemory.resize(localStack.back().previousSize);
	localStack.pop_back();
	dataMemories[MemoryPlace::Type::Local] = localMemory.data() + localStack.back().previousSize;
}

TypeBase::Ref& Executor::get(const MemoryPlace& place)
{
	return dataMemories[place.memoryType][place.memoryIndex];
}

void Executor::set(const MemoryPlace& place, TypeBase::Ref value)
{
	get(place) = std::move(value);
}

void Executor::swap(const MemoryPlace& place0, const MemoryPlace& place1)
{
	std::swap(get(place0), get(place1));
}

void Executor::set(OperationModule* opModule)
{
	currentModule = opModule;
	dataMemories[MemoryPlace::Type::Module] = currentModule != nullptr ? currentModule->data.data() : nullptr;
}


void ExecutorOld::run()
{
	while (currentCode != nullptr)
		currentCode->getOperation().execute(*this);
}

void ExecutorOld::runIt()
{
	while (currentCode != nullptr)
		currentCode->getOperationIt().execute(*this);
}

void ExecutorOld::runFib()
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

void ExecutorOld::runFibNC()
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
		const auto& add0 = static_cast<const AddOperationOld&>(*currentCode->operations[4]);
		*get(add0.destinationIndex) += *get(add0.sourceIndex);
		const auto& swap0 = static_cast<const SwapOperation&>(*currentCode->operations[5]);
		std::swap(get(swap0.index0), get(swap0.index1));
		const auto& add1 = static_cast<const AddOperationOld&>(*currentCode->operations[6]);
		*get(add1.destinationIndex) += *get(add1.sourceIndex);
		const auto& grt0 = static_cast<const GreaterOperation&>(*currentCode->operations[7]);
		set(grt0.destinationIndex, *get(grt0.sourceIndex1) < *get(grt0.sourceIndex0) ? TypeBool::trueValue : TypeBool::falseValue);
	} 	while (get(5)->as<TypeBool::ValueType>());
}

void ExecutorOld::runFibNOP()
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

void ExecutorOld::runFibNOP2()
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

void ExecutorOld::runFibNOP3()
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

void ExecutorOld::pushCode(const std::vector<OperationOld::Ref>& code)
{
	codeStack.emplace_back(code, size());
	currentCode = &codeStack.back();
}

void ExecutorOld::popCode()
{
	codeStack.pop_back();
	currentCode = codeStack.empty() ? nullptr : &codeStack.back();
}

CodeBlock& ExecutorOld::getCode()
{
	return *currentCode;
}

TypeBase::Ref& ExecutorOld::get(size_t index)
{
	_ASSERT(index < size());
	return dataFrame[index];
}

void ExecutorOld::set(size_t index, TypeBase::Ref value)
{
	_ASSERT(index < size());
	dataFrame[index] = value;
}

size_t ExecutorOld::size() const
{
	return dataFrame.size();
}

void ExecutorOld::resize(size_t size)
{
	dataFrame.resize(size);
}


CodeBlock::CodeBlock(const std::vector<OperationOld::Ref>& code, size_t bottom)
	: operations(code), currentOperation(operations.begin()), dataBottom(bottom)
{}

const OperationOld& CodeBlock::getOperation()
{
	return *operations[currentOperationIdx++];
}

const OperationOld& CodeBlock::getOperationIt()
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