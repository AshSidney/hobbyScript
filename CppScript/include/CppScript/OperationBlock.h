#pragma once

#include <CppScript/Definitions.h>
#include <CppScript/Value.h>
#include <CppScript/Operation.h>
#include <CppScript/Allocator.h>
#include <array>
#include <vector>
#include <span>

namespace CppScript
{

class CPPSCRIPT_API OperationBlockLayout
{
public:
	OperationBlockLayout(std::vector<const TypeId*> valTypes, std::vector<const std::vector<ValuePlace>*> argPlaces);
	
	std::vector<const TypeId*> valueTypes;
	TypeId::Layout valuesLayout;
	std::vector<std::size_t> valuesOrder;
	std::size_t valuesDestructCount{ 0 };
	std::vector<const std::vector<ValuePlace>*> argumentPlaces;
	TypeId::Layout fullLayout;
	
	static constexpr TypeId::Layout valuePtrLayout{ TypeId::Layout::create<ValueBase*>() };
	static constexpr TypeId::Layout argumentPtrLayout{ TypeId::Layout::create<Arguments>() };

private:
	void addType(const TypeId& typeId);
	std::size_t countArguments() const;
};


class CPPSCRIPT_API ValuesFrame
{
public:
    ValuesFrame() = default;
    ~ValuesFrame() noexcept;
    ValuesFrame(ValuesFrame&&) noexcept;
    ValuesFrame(const ValuesFrame&) = delete;
    ValuesFrame& operator=(ValuesFrame&&) = delete;
    ValuesFrame& operator=(const ValuesFrame&) = delete;

	std::byte* initialize(std::byte* buffer, const OperationBlockLayout& layout);
	
    ValueBase& get(const std::size_t index)
	{
		assert(index < values.size());
		return *values[index];
	}
		
private:
	std::span<ValueBase*> values;
	std::span<ValueBase*> destructValues;
};


struct OperationBlockContext
{
	std::array<ValuesFrame*, EnumTraits<ValuePlace::Type>::index(ValuePlace::Type::Void)> frames;
};


class CPPSCRIPT_API OperationsArgumentsFrame
{
public:
	OperationsArgumentsFrame() = default;
	~OperationsArgumentsFrame() noexcept = default;
	OperationsArgumentsFrame(OperationsArgumentsFrame&&) noexcept = default;
	OperationsArgumentsFrame(const OperationsArgumentsFrame&) = delete;
	OperationsArgumentsFrame& operator=(OperationsArgumentsFrame&&) = delete;
	OperationsArgumentsFrame& operator=(const OperationsArgumentsFrame&) = delete;
	
	std::byte* initialize(std::byte* buffer, const OperationBlockLayout& layout, OperationBlockContext& context);
	
	Arguments get(const std::size_t index)
	{
		return arguments[index];
	}
	
private:
	std::span<Arguments> arguments;
};


template <typename O>
class OperationBlock;

template <typename O, typename A>
class OperationFrame
{
public:
	OperationFrame(std::span<O> operations, const OperationBlockLayout& layout,
		OperationBlockContext& context)
		: operations(operations),
		buffer(Allocator<A>::allocate(layout.fullLayout))
	{
		std::byte* argsPtr = values.initialize(buffer.get(), layout);
		context.frames[EnumTraits<ValuePlace::Type>::index(ValuePlace::Type::Local)] = &values;
		[[maybe_unused]] std::byte* endPtr = arguments.initialize(argsPtr, layout, context);
		assert(endPtr <= buffer.get() + layout.fullLayout.size);
	}

	void execute()
	{
		while (currentOperation < operations.size())
		{
			context.arguments = arguments.get(currentOperation);
			operations[currentOperation].execute(context);
			currentOperation += context.nextStep;
		}
	}

	ValuesFrame& getValues()
	{
		return values;
	}

	OperationContext& getContext()
	{
		return context;
	}

	void restart()
	{
		currentOperation = 0;
	}

private:
	std::span<O> operations;
	Allocator<A>::MemoryPtr buffer;
	ValuesFrame values;
	OperationsArgumentsFrame arguments;
	std::size_t currentOperation{ 0 };
	OperationContext context;
};


template <typename O>
class OperationBlock
{
public:
	OperationBlock(std::vector<const TypeId*> valTypes, std::vector<O> operations)
		: operations(std::move(operations)),
		layout(std::move(valTypes), getArgumentPlaces(this->operations))
	{}

	template <typename A = DefaultAllocator>
	OperationFrame<O, A> createFrame(OperationBlockContext& context)
	{
		return OperationFrame<O, A>({ operations.begin(), operations.end() }, layout, context);
	}

private:
	std::vector<O> operations;
	OperationBlockLayout layout;
};

}