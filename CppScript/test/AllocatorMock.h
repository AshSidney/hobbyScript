#pragma once

#include <gmock/gmock.h>
#include <CppScript/Value.h>
#include <CppScript/Allocator.h>

namespace CppScriptTest
{

class AllocMock
{
public:
    AllocMock();
    ~AllocMock();

    void expectAllocFree(const CppScript::TypeId::Layout& layout, std::byte* memPtr, bool alignedAlloc);

    MOCK_METHOD((std::tuple<std::byte*, bool>), alloc, (const CppScript::TypeId::Layout& layout), (const));
    MOCK_METHOD(void, free, (std::byte* memPtr, bool alignedAlloc), (const));

	static std::tuple<std::byte*, bool> allocMem(const CppScript::TypeId::Layout& layout);
    static void freeMem(std::byte* memPtr, bool alignedAlloc) noexcept;

private:  
    static AllocMock* instance;
};

using AllocatorMock = CppScript::Allocator<AllocMock>;

bool isAligned(const std::byte* ptr, std::size_t alignment);

template <typename A>
bool isAligned(const typename CppScript::Allocator<A>::MemoryPtr& memPtr, const std::size_t alignment)
{
    return isAligned(memPtr.get(), alignment);
}

}
