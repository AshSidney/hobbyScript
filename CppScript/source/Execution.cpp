#include <CppScript/Execution.h>
#include <CppScript/FunctionDef.h>

namespace CppScript
{

void CodeBlock::registerCache(PlaceDataCache& cache)
{
    auto& cacheType = caches[PlaceData::typeIndex(cache.argType)];
    if (cache.offset >= cacheType.size())
        cacheType.resize(cache.offset + 1);
    cacheType[cache.offset].push_back(&cache);
}


void ExecutionContext::run(CodeBlock& code)
{
    this->code = &code;
    refreshCache(PlaceType::Local, PlaceType::Argument);
    
    for (codePointer = this->code->operations.cbegin(); codePointer != this->code->operations.cend(); codePointer += nextCodeOffset)
    {
        nextCodeOffset = codeStep;
        (*codePointer)->execute(*this);
    }
}

void ExecutionContext::set(const PlaceData& place, ValueHolder& val)
{
    auto& pointers = placePointers[PlaceData::typeIndex(place.argType)];
    if (pointers.size() <= place.offset)
        pointers.resize(place.offset + 1);
    pointers[place.offset] = &val;
}

void ExecutionContext::refreshCache(const PlaceType startType, const PlaceType endType)
{
    for (size_t currType = PlaceData::typeIndex(startType); currType <= PlaceData::typeIndex(endType); ++currType)
    {
        const auto& currCaches = code->caches[currType];
        for (size_t index = 0; index < currCaches.size(); ++index)
        {
            auto* valueHolder = placePointers[currType][index];
            for (auto* cache : currCaches[index])
                cache->setHolder(*valueHolder);
        }
    }
}

}