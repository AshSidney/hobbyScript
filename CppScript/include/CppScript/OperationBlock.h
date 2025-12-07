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
    ~ValuesFrame() noexcept = default;
    ValuesFrame(ValuesFrame&& source) noexcept;
    ValuesFrame(const ValuesFrame&) = delete;
    ValuesFrame& operator=(ValuesFrame&&) = delete;
    ValuesFrame& operator=(const ValuesFrame&) = delete;

	std::span<ValueBase*> getValues()
	{
		return values;
	}
	
    ValueBase& get(const std::size_t index)
	{
		assert(index < values.size());
		return *values[index];
	}

protected:
	std::span<ValueBase*> values;
};

class CPPSCRIPT_API VariablesFrame : public ValuesFrame
{
public:
    VariablesFrame() = default;
    ~VariablesFrame() noexcept;
	VariablesFrame(VariablesFrame&& source) noexcept;

	std::byte* initialize(std::byte* buffer, const OperationBlockLayout& layout);
		
private:
	std::span<ValueBase*> destructValues;
};

class CPPSCRIPT_API ConstantsFrame : public ValuesFrame
{
public:
    ConstantsFrame(std::vector<std::unique_ptr<ValueBase>> constVals);
    ~ConstantsFrame() noexcept;
	ConstantsFrame(ConstantsFrame&& source) noexcept;

private:
	std::vector<ValueBase*> constValues;
};

class CPPSCRIPT_API ArgumentsFrame : public ValuesFrame
{
public:
    ArgumentsFrame();
};


struct OperationBlockContext
{
	std::array<std::span<ValueBase*>, framesCount> frames;
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
		getFrame(context.frames, ValuePlace::Type::Local) = values.getValues();
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
	VariablesFrame values;
	OperationsArgumentsFrame arguments;
	std::size_t currentOperation{ 0 };
	OperationContext context;
};


template <typename OB>
class OperationBlock
{
public:
	OperationBlock(const OB& builder, OperationBlockBuildContext source)
		: operations(builder.build(source.operations)),
		layout(std::move(source.valueTypes), getArgumentPlaces(operations))
	{
		if (!source.constantValues.empty())
			constantsFrame.emplace(std::move(source.constantValues));
	}

	OperationBlockContext createContext()
	{
		OperationBlockContext context;
		if (constantsFrame)
			getFrame(context.frames, ValuePlace::Type::Constants) = constantsFrame->getValues();
		return context;
	}

	using OperationType = typename OB::OperationType;
	
	template <typename A = DefaultAllocator>
	OperationFrame<OperationType, A> createFrame(OperationBlockContext& context)
	{
		return OperationFrame<OperationType, A>({ operations.begin(), operations.end() }, layout, context);
	}

private:
	std::vector<OperationType> operations;
	OperationBlockLayout layout;
	std::optional<ConstantsFrame> constantsFrame;
};

}