#include "AllocatorMock.h"
#include "TestUtils.h"

namespace CppScriptTest
{

AllocMock* AllocMock::instance { nullptr };

AllocMock::AllocMock()
{
    EXPECT_EQ(instance, nullptr);
    instance = this;
}

AllocMock::~AllocMock()
{
    EXPECT_EQ(instance, this);
    instance = nullptr;
}

void AllocMock::expectAllocFree(const CppScript::TypeId::Layout& layout, std::byte* memPtr, bool alignedAlloc)
{
    EXPECT_CALL(*this, alloc(layout))
        .WillOnce(testing::Return(std::make_tuple(memPtr, alignedAlloc)));
    EXPECT_CALL(*this, free(memPtr, alignedAlloc))
        .Times(1);
}

std::tuple<std::byte*, bool> AllocMock::allocMem(const CppScript::TypeId::Layout& layout)
{
    EXPECT_NE(instance, nullptr);
    return instance->alloc(layout);
}

void AllocMock::freeMem(std::byte* memPtr, bool alignedAlloc) noexcept
{
    EXPECT_NE(instance, nullptr);
    instance->free(memPtr, alignedAlloc);
}

bool isAligned(const std::byte* ptr, const std::size_t alignment)
{
    return CppScript::Alignment::isAligned(reinterpret_cast<std::size_t>(ptr), alignment);
}



}