#pragma once

#include <CppScript/Execution.h>

namespace CppScript
{

void* ExecutionContext::get(const MemoryPlace& place) const
{
    return memoryPointers[memoryTypeIndex(place.memoryType)][place.index];
}

void ExecutionContext::set(const MemoryPlace& place, void* ptr)
{
    auto& pointers = memoryPointers[memoryTypeIndex(place.memoryType)];
    if (pointers.size() <= place.index)
        pointers.resize(place.index + 1);
    pointers[place.index] = ptr;
}

}