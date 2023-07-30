#include <CppScript/Execution.h>
#include <CppScript/FunctionDef.h>
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


DataBlock::DataBlock(const PlaceType type) : placesType(type)
{}

PlaceData DataBlock::addPlace(PlaceTypeOffset place)
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

const TypeId* DataBlock::getPlaceType(const size_t index) const
{
    return index < placeTypeOffsets.size() ? placeTypeOffsets[index].typeId->basicTypeId : nullptr;
}


CodeBlock::CodeBlock() : DataBlock(PlaceType::Local)
{}

void CodeBlock::registerCache(PlaceDataCache& cache)
{
    auto& cacheType = caches[PlaceData::typeIndex(cache.argType)];
    if (cache.index >= cacheType.size())
        cacheType.resize(cache.index + 1);
    cacheType[cache.index].push_back(&cache);
}

void ExecutionContext::run(CodeBlock& code)
{
    this->code = &code;
    memoryBlocks[PlaceData::typeIndex(code.getType())] = code.makeMemoryBlock<>();
    refreshCache(PlaceType::Local, PlaceType::Local);
    run();
}

void ExecutionContext::run()
{
    assert(code != nullptr);
    for (codePointer = this->code->operations.cbegin(); codePointer != this->code->operations.cend(); codePointer += nextCodeOffset)
    {
        nextCodeOffset = codeStep;
        (*codePointer)->execute(*this);
    }
}

void ExecutionContext::refreshCache(const PlaceType startType, const PlaceType endType)
{
    for (size_t currType = PlaceData::typeIndex(startType); currType <= PlaceData::typeIndex(endType); ++currType)
    {
        const auto& currCaches = code->caches[currType];
        for (size_t index = 0; index < currCaches.size(); ++index)
        {
            auto& valueHolder = memoryBlocks[currType].get(index);
            for (auto* cache : currCaches[index])
                cache->setHolder(valueHolder);
        }
    }
}

}