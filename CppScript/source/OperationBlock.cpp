#include <CppScript/OperationBlock.h>

namespace
{

template <typename T>
inline T* createPtrArray(std::byte* buffer, const std::size_t offset, const std::size_t count)
{
    assert(std::memset(buffer + offset, 0, count * CppScript::TypeId::Layout::create<T>().size) != nullptr);
    assert(CppScript::Alignment::isAligned(reinterpret_cast<std::size_t>(buffer + offset), CppScript::TypeId::Layout::create<T>().alignment));
	return reinterpret_cast<T*>(buffer + offset);
}

template <typename T>
inline std::span<T> createSpan(std::byte* buffer, const std::size_t offset, const std::size_t count)
{
	return { createPtrArray<T>(buffer, offset, count), count };
}

}

namespace CppScript
{

OperationBlockLayout::OperationBlockLayout(std::vector<const TypeId*> valTypes, std::vector<const std::vector<ValuePlace>*> argPlaces)
	: valueTypes(std::move(valTypes)), argumentPlaces(std::move(argPlaces))
{
    valuesOrder.reserve(valueTypes.size());
    for (const auto* typeId : valueTypes)
        addType(*typeId);
    fullLayout = Alignment::alignUp(valuesLayout + valuePtrLayout * (valueTypes.size() + valuesDestructCount + countArguments())
        + argumentPtrLayout * argumentPlaces.size());
}

void OperationBlockLayout::addType(const TypeId& typeId)
{
    auto backIt = valuesOrder.rbegin();
    if (!Alignment::isAligned(valuesLayout.size, typeId.layout.alignment))
    {
        std::size_t backOffset = valuesLayout.size;
        do
        {
            backOffset -= valueTypes[*backIt]->layout.size;
            ++backIt;
            assert((backIt == valuesOrder.rend()) == (backOffset == 0));
        }
        while (!Alignment::isAligned(backOffset, typeId.layout.alignment));
    }
    valuesOrder.insert(backIt.base(), valuesOrder.size());

    valuesLayout += typeId.layout;

    if (!typeId.isReference())
        ++valuesDestructCount;
}

std::size_t OperationBlockLayout::countArguments() const
{
    return std::accumulate(argumentPlaces.cbegin(), argumentPlaces.cend(), std::size_t(0),
        [](const std::size_t acc, const auto* args)
        {
        	return acc + args->size();
        });
}


ValuesFrame::ValuesFrame(ValuesFrame&& source) noexcept
{
    std::swap(values, source.values);
}

VariablesFrame::~VariablesFrame() noexcept
{
    for (ValueBase* value : destructValues)
        value->~ValueBase();
}

VariablesFrame::VariablesFrame(VariablesFrame&& source) noexcept : ValuesFrame(std::move(source))
{
    std::swap(destructValues, source.destructValues);
}

std::byte* VariablesFrame::initialize(std::byte* buffer, const OperationBlockLayout& layout)
{
    assert(values.empty());
    assert(Alignment::isAligned(reinterpret_cast<std::size_t>(buffer), layout.valuesLayout.alignment));
	assert(layout.valueTypes.size() == layout.valuesOrder.size());
	assert(layout.valuesDestructCount <= layout.valueTypes.size());
    const std::size_t valueListOffset = Alignment::alignUp(layout.valuesLayout.size, OperationBlockLayout::valuePtrLayout.alignment);
    values = createSpan<ValueBase*>(buffer, valueListOffset, layout.valueTypes.size());
	const std::size_t destructListOffset = valueListOffset + values.size() * OperationBlockLayout::valuePtrLayout.size;
	destructValues = createSpan<ValueBase*>(buffer, destructListOffset, layout.valuesDestructCount);
    std::byte* currentBuffer = buffer;
    std::size_t destructIndex{ 0 };
    for (const auto index : layout.valuesOrder)
    {
		assert(values[index] == nullptr);
        const TypeId& typeId = *layout.valueTypes[index];
        values[index] = typeId.create(currentBuffer);
        currentBuffer += typeId.layout.size;
        if (!typeId.isReference())
			destructValues[destructIndex++] = values[index];
    }
    assert(currentBuffer <= buffer + valueListOffset);
	assert(destructIndex == layout.valuesDestructCount);
	const std::size_t endOffset = destructListOffset + destructValues.size() * OperationBlockLayout::valuePtrLayout.size;
    return buffer + endOffset;
}

ConstantsFrame::ConstantsFrame(std::vector<std::unique_ptr<ValueBase>> constVals)
{
    constValues.reserve(constVals.size());
    std::transform(constVals.begin(), constVals.end(), std::back_inserter(constValues),
        [](std::unique_ptr<ValueBase>& val){ return val.release(); });
    values = { constValues.begin(), constValues.end() };
}

ConstantsFrame::~ConstantsFrame() noexcept
{
    for (ValueBase* value : constValues)
        delete value;
}

ConstantsFrame::ConstantsFrame(ConstantsFrame&& source) noexcept : ValuesFrame(std::move(source))
{
    std::swap(constValues, source.constValues);
}


std::byte* OperationsArgumentsFrame::initialize(std::byte* buffer, const OperationBlockLayout& layout, OperationBlockContext& context)
{
    assert(arguments.empty());
    arguments = createSpan<Arguments>(buffer, 0, layout.argumentPlaces.size());
    std::size_t currentOffset = Alignment::alignUp(arguments.size() * OperationBlockLayout::argumentPtrLayout.size,
        OperationBlockLayout::valuePtrLayout.alignment);
    for (std::size_t index = 0; index < arguments.size(); ++index)
    {
        assert(layout.argumentPlaces[index] != nullptr);
        const std::vector<ValuePlace>& argPlaces = *layout.argumentPlaces[index];
        ValueBase** argIt = createPtrArray<ValueBase*>(buffer, currentOffset, argPlaces.size());
        arguments[index] = argIt;
        for (const ValuePlace place : argPlaces)
        {
            if (const std::size_t frameIndex = EnumTraits<ValuePlace::Type>::index(place.placeType); frameIndex < context.frames.size())
                *argIt = context.frames[frameIndex][place.index];
            else
                *argIt = nullptr;
            ++argIt;
        }
        currentOffset += argPlaces.size() * OperationBlockLayout::valuePtrLayout.size;
    }
	return buffer + currentOffset;
}

}