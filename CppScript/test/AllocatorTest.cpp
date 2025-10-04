#include <gmock/gmock.h>

#include <CppScript/Allocator.h>
#include "AllocatorMock.h"
#include "TestUtils.h"

using namespace CppScript;
using namespace CppScriptTest;

constexpr std::size_t limitAlignment{ alignof(std::max_align_t) };


TEST(AllocatorTest, AllocWithStandardAlignment)
{
    const std::size_t smallAlign{ 2 };
    ASSERT_LT(smallAlign, limitAlignment);
    const auto [memPtrSmallAlign, smallAlignAlloc] = DefaultAllocator::allocMem({24, smallAlign});
    EXPECT_NE(memPtrSmallAlign, nullptr);
    EXPECT_TRUE(isAligned(memPtrSmallAlign, limitAlignment));
    EXPECT_FALSE(smallAlignAlloc);
    DefaultAllocator::freeMem(memPtrSmallAlign, smallAlignAlloc);
    
    const auto [memPtrLimitAlign, limitAlignAlloc] = DefaultAllocator::allocMem({5 * limitAlignment, limitAlignment});
    EXPECT_NE(memPtrLimitAlign, nullptr);
    EXPECT_TRUE(isAligned(memPtrLimitAlign, limitAlignment));
    EXPECT_FALSE(limitAlignAlloc);
    DefaultAllocator::freeMem(memPtrLimitAlign, limitAlignAlloc);
}

TEST(AllocatorTest, AllocWithLargeAlignment)
{
    const std::size_t largeAlign{ 1024 };
    ASSERT_GT(largeAlign, limitAlignment);
    const auto [memPtrLargeAlign, largeAlignAlloc] = DefaultAllocator::allocMem({2 * largeAlign, largeAlign});
    EXPECT_NE(memPtrLargeAlign, nullptr);
    EXPECT_TRUE(isAligned(memPtrLargeAlign, largeAlign));
    EXPECT_TRUE(largeAlignAlloc);
    DefaultAllocator::freeMem(memPtrLargeAlign, largeAlignAlloc);
}

TEST(AllocatorTest, AllocBufferWithStandardAlignment)
{
    std::byte buffer[1024];
    TypeId::Layout layout{ 3 * limitAlignment, limitAlignment };
    AllocMock allocMock;
    allocMock.expectAllocFree(layout, buffer, false);
    auto allocBuffer = AllocatorMock::allocate(layout);
    EXPECT_EQ(allocBuffer.get(), buffer);
}

TEST(AllocatorTest, AllocBufferWithLargeAlignment)
{
    std::byte buffer[1024];
    TypeId::Layout layout{ 12 * limitAlignment, 4 * limitAlignment };
    AllocMock allocMock;
    allocMock.expectAllocFree(layout, buffer, true);
    auto allocBuffer = AllocatorMock::allocate(layout);
    EXPECT_EQ(allocBuffer.get(), buffer);
    allocBuffer.reset();
}

TEST(AllocatorTest, IsPowerTwo)
{
    EXPECT_TRUE(Alignment::isPowerTwo(0));
    EXPECT_TRUE(Alignment::isPowerTwo(1));
    EXPECT_TRUE(Alignment::isPowerTwo(2));
    EXPECT_FALSE(Alignment::isPowerTwo(3));
    EXPECT_TRUE(Alignment::isPowerTwo(4));
    EXPECT_FALSE(Alignment::isPowerTwo(5));
    EXPECT_FALSE(Alignment::isPowerTwo(6));
    EXPECT_FALSE(Alignment::isPowerTwo(7));
    EXPECT_TRUE(Alignment::isPowerTwo(8));
    EXPECT_TRUE(Alignment::isPowerTwo(0x10));
    EXPECT_FALSE(Alignment::isPowerTwo(0x14));
    EXPECT_TRUE(Alignment::isPowerTwo(0x20));
    EXPECT_FALSE(Alignment::isPowerTwo(0x48));
    EXPECT_TRUE(Alignment::isPowerTwo(0x80));
}

TEST(AllocatorTest, AlignDown)
{
    EXPECT_EQ(Alignment::alignDown(100, 8), 96);
    EXPECT_EQ(Alignment::alignDown(120, 16), 112);
    EXPECT_EQ(Alignment::alignDown(231, 32), 224);
    EXPECT_EQ(Alignment::alignDown(160, 8), 160);
}

TEST(AllocatorTest, AlignUp)
{
    EXPECT_EQ(Alignment::alignUp(100, 8), 104);
    EXPECT_EQ(Alignment::alignUp(120, 16), 128);
    EXPECT_EQ(Alignment::alignUp(231, 32), 256);
    EXPECT_EQ(Alignment::alignUp(160, 8), 160);
}

TEST(AllocatorTest, IsAligned)
{
    EXPECT_FALSE(Alignment::isAligned(180, 8));
    EXPECT_FALSE(Alignment::isAligned(428, 16));
    EXPECT_FALSE(Alignment::isAligned(231, 32));
    EXPECT_TRUE(Alignment::isAligned(184, 8));
}

TEST(AllocatorTest, AlignUpLayoutExplicitAlignment)
{
    EXPECT_EQ(Alignment::alignUp({100, 8}, 16), TypeId::Layout(112, 16));
    EXPECT_EQ(Alignment::alignUp({130, 16}, 16), TypeId::Layout(144, 16));
    EXPECT_EQ(Alignment::alignUp({231, 32}, 8), TypeId::Layout(232, 32));
    EXPECT_EQ(Alignment::alignUp({160, 8}, 16), TypeId::Layout(160, 16));
}

TEST(AllocatorTest, AlignUpLayout)
{
    EXPECT_EQ(Alignment::alignUp({100, 8}), TypeId::Layout(104, 8));
    EXPECT_EQ(Alignment::alignUp({130, 16}), TypeId::Layout(144, 16));
    EXPECT_EQ(Alignment::alignUp({231, 32}), TypeId::Layout(256, 32));
    EXPECT_EQ(Alignment::alignUp({160, 8}), TypeId::Layout(160, 8));
}
