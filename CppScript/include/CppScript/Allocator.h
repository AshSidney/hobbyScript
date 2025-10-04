#pragma once

#include <CppScript/Definitions.h>
#include <CppScript/Value.h>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <tuple>
#include <cassert>

namespace CppScript
{

class CPPSCRIPT_API Alignment
{
public:
    static constexpr bool isPowerTwo(const std::size_t alignment)
    {
        return (alignment & (alignment - 1)) == 0;
    }

	static constexpr size_t alignDown(const std::size_t offset, const std::size_t alignment)
	{
		assert(isPowerTwo(alignment));
		return offset & (~(alignment - 1));
	}

	static constexpr std::size_t alignUp(const std::size_t offset, const std::size_t alignment)
	{
		std::size_t alignedOffset = alignDown(offset, alignment);
		if (offset != alignedOffset)
			alignedOffset += alignment;
		return alignedOffset;
	}

    static constexpr bool isAligned(const std::size_t offset, const std::size_t alignment)
	{
		return offset == alignDown(offset, alignment);
	}

	static constexpr TypeId::Layout alignUp(const TypeId::Layout& layout, const std::size_t alignment)
	{
		return { alignUp(layout.size, alignment), std::max(layout.alignment, alignment) };
	}

	static constexpr TypeId::Layout alignUp(const TypeId::Layout& layout)
	{
		return alignUp(layout, layout.alignment);
	}
};


template <typename A>
class Allocator
{
public:
    class Deleter
    {
    public:
		explicit Deleter(bool aligned) : alignedAlloc(aligned)
		{}

        void operator()(std::byte* memPtr) const noexcept
        {
            A::freeMem(memPtr, alignedAlloc);
        }

	private:
		bool alignedAlloc{ false };
    };

	using MemoryPtr = std::unique_ptr<std::byte, Deleter>;

	static MemoryPtr allocate(const TypeId::Layout& layout)
	{
		assert(Alignment::isAligned(layout.size, layout.alignment));
		const auto [memPtr, aligned] = A::allocMem(layout);
		return { memPtr, Deleter{ aligned } };
	}
};


class CPPSCRIPT_API DefaultAllocator
{
public:
	static std::tuple<std::byte*, bool> allocMem(const TypeId::Layout& layout);
	static void freeMem(std::byte* memPtr, bool alignedAlloc) noexcept;
};

using CommonAllocator = Allocator<DefaultAllocator>;

}