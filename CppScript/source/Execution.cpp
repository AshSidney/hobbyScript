#include <CppScript/Execution.h>
#include <CppScript/FunctionDef.h>
#include <CppScript/ScriptModule.h>
#include <algorithm>
#include <iterator>
#include <cstdlib>
#include <cassert>

namespace CppScript
{

std::byte* MemoryAllocator::allocate(const TypeLayout& layout)
{
    assert(isAligned(layout.size, layout.alignment));
#ifdef _MSC_VER
    void* mem = _aligned_malloc(layout.size, layout.alignment);
#else
    void* mem = aligned_alloc(layout.alignment, layout.size);
#endif
    return static_cast<std::byte*>(mem);
}

void MemoryAllocator::free(std::byte* memPtr)
{
#ifdef _MSC_VER
    _aligned_free(memPtr);
#else
    free(memPtr);
#endif
}


DataBlockDef DataBlockDef::Builder::build()
{
    const auto valuePtrs = TypeLayout::make<ValueHolder*>(places.size() + destructCount);
    assert(blockLayout.size == 0 || blockLayout.alignment >= valuePtrs.alignment);
    DataBlockDef result{ { MemoryAllocator::alignUp(blockLayout + valuePtrs), {},
        blockLayout.size, destructCount }, std::move(values) };
    result.layout.values.reserve(places.size());
    for (const auto& place : places)
        result.layout.values.emplace_back(*place.typeId, place.offset, place.source);
    clear();
    return result;
}

size_t DataBlockDef::Builder::addPlace(const TypeId& typeId)
{
    return addPlace(typeId, nullptr);
}

size_t DataBlockDef::Builder::addPlace(std::unique_ptr<ValueHolder> value)
{
    const ValueHolder* valuePtr = value.get();
    values.push_back(std::move(value));
    const TypeId& typeId = *valuePtr->getSpecTypeId().refTypeId;
    return addPlace(typeId, valuePtr);
}

size_t DataBlockDef::Builder::addPlace(const TypeId& typeId, const ValueHolder* value)
{
    const TypeLayout typeLayout = typeId.layout;
    assert(MemoryAllocator::isAligned(typeLayout.size, typeLayout.alignment));
    const size_t valueIndex = places.size();
    placeIndices.push_back(valueIndex);
    size_t placeOffset = blockLayout.size;
    if (!MemoryAllocator::isAligned(placeOffset, typeLayout.alignment))
    {
        for (size_t index = valueIndex; index > 0; --index)
        {
            const size_t prevIndex = placeIndices[index - 1];
            placeIndices[index] = prevIndex;
            placeOffset = places[prevIndex].offset;
            places[prevIndex].offset += typeLayout.size;
            if (MemoryAllocator::isAligned(placeOffset, typeLayout.alignment))
            {
                placeIndices[index - 1] = valueIndex;
                break;
            }
        }
    }
    places.push_back({ &typeId, placeOffset, value });
    blockLayout += typeId.layout;
    if (!typeId.isReference)
            ++destructCount;
    return valueIndex;
}

void DataBlockDef::Builder::clear()
{
    blockLayout = {};
    places.clear();
    values.clear();
    placeIndices.clear();
    destructCount = 0;
}


DataBlockOld::DataBlockOld(const PlaceType type) : placesType(type)
{}


PlaceData DataBlockOld::addPlace(PlaceTypeOffset place)
{
    const TypeLayout typeLayout = place.typeId->layout;
    assert(MemoryAllocator::isAligned(typeLayout.size, typeLayout.alignment));
    const size_t placeIndex = placeTypeOffsets.size();
    placeIndices.push_back(placeIndex);
    placeTypeOffsets.push_back(std::move(place));
    size_t& placeOffset = placeTypeOffsets[placeIndex].offset;
    placeOffset = blockLayout.size;
    if (!MemoryAllocator::isAligned(placeOffset, typeLayout.alignment))
    {
        for (size_t index = placeIndex; index > 0; --index)
        {
            size_t& currIndex = placeIndices[index];
            size_t& prevIndex = placeIndices[index - 1];
            currIndex = prevIndex;
            placeOffset = placeTypeOffsets[currIndex].offset;
            placeTypeOffsets[currIndex].offset += typeLayout.size;
            if (MemoryAllocator::isAligned(placeOffset, typeLayout.alignment))
            {
                prevIndex = placeIndex;
                break;
            }
        }
    }
    blockLayout += typeLayout;
    return { placesType, placeIndex };
}

const TypeId* DataBlockOld::getPlaceType(const size_t index) const
{
    return index < placeTypeOffsets.size() ? placeTypeOffsets[index].typeId->basicTypeId : nullptr;
}


CodeBlock::CodeBlock(DataBlockDef data) : dataDef(std::move(data))
{}

void CodeBlock::registerCache(PlaceDataCache& cache)
{
    auto& cacheType = caches[PlaceData::typeIndex(cache.argType)];
    if (cache.index >= cacheType.size())
        cacheType.resize(cache.index + 1);
    cacheType[cache.index].push_back(&cache);
}


void ExecutionContext::run(const CodeBlock& code, DataBlock<>& data)
{
    currentData = { nullptr, &data };
    codeStack.push_back({ DataBlock<>{{}}, &data, &code });
    currentFrame = &codeStack.back();
    refreshCache(PlaceType::Module);
    currentFrame->run(*this);
    codeStack.pop_back();
}

void ExecutionContext::run(const ScriptFunction& func)
{}

void ExecutionContext::run(const ScriptModule& module)
{
    if (module.getData() == nullptr)
    {
        const CodeBlock& code = module.getCode();
        module.setData(moduleData.emplace_back(code.getDataLayout()));
        run(code, *module.getData());
    }
}

void ExecutionContext::refreshCache(const PlaceType placeType)
{
    const size_t typeIndex = PlaceData::typeIndex(placeType);
    const auto& currCaches = currentFrame->code->caches[typeIndex];
    for (size_t index = 0; index < currCaches.size(); ++index)
    {
        auto& valueHolder = currentData[typeIndex]->get(index);
        for (auto* cache : currCaches[index])
            cache->setHolder(valueHolder);
    }
}

void ExecutionContext::CodeFrame::run(ExecutionContext& context)
{
    assert(code != nullptr);
    const auto codeEnd = code->operations.cend();
    for (codePointer = code->operations.cbegin(); codePointer != codeEnd; codePointer += nextCodeOffset)
    {
        nextCodeOffset = codeStep;
        (*codePointer)->execute(context);
    }
}

}