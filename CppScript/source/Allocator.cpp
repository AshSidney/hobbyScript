#include <CppScript/Allocator.h>
#include <cstdlib>

namespace CppScript
{

std::tuple<std::byte*, bool> DefaultAllocator::allocMem(const TypeId::Layout& layout)
{
    void* memPtr{ nullptr };
    const bool alignedAlloc { layout.alignment > alignof(std::max_align_t) };
    if (alignedAlloc)
    {
#ifdef _MSC_VER
        memPtr = _aligned_malloc(layout.size, layout.alignment);
#else
        memPtr = aligned_alloc(layout.alignment, layout.size);
#endif
    }
    else
    {
       memPtr = malloc(layout.size);
    }
    return { static_cast<std::byte*>(memPtr), alignedAlloc };
}

void DefaultAllocator::freeMem(std::byte* memPtr, const bool alignedAlloc) noexcept
{
#ifdef _MSC_VER
    if (alignedAlloc)
        _aligned_free(memPtr);
    else
        free(memPtr);
#else
    free(memPtr);
#endif
}

}