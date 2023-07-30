#pragma once

#include <CppScript/Definitions.h>
#include <CppScript/ValueHolder.h>
#include <memory>
#include <array>
#include <vector>
#include <cassert>


namespace CppScript
{

enum class PlaceType
{
	Local,
	Module,
	Void
};


class CPPSCRIPT_API PlaceData
{
public:
	PlaceType argType;
	size_t index;

	static constexpr size_t typeIndex(const PlaceType argType)
	{
		return static_cast<size_t>(argType);
	}
};

class CPPSCRIPT_API PlaceDataCache : public PlaceData
{
public:
    void setHolder(ValueHolder& holder)
    {
        valueHolder = &holder;
    }

protected:
    ValueHolder* valueHolder{ nullptr };
};


class CPPSCRIPT_API MemoryAllocator
{
public:
	static std::byte* allocate(const TypeLayout& layout);
	static void free(std::byte* memPtr);

    static constexpr bool isPowerTwo(const size_t alignment)
    {
        return (alignment & (alignment - 1)) == 0;
    }

	static constexpr size_t alignDown(size_t offset, size_t alignment)
	{
		assert(isPowerTwo(alignment));
		return offset & (~(alignment - 1));
	}

	static constexpr size_t alignUp(size_t offset, size_t alignment)
	{
		size_t aligned = alignDown(offset, alignment);
		if (offset != aligned)
			aligned += alignment;
		return aligned;
	}

    static constexpr bool isAligned(size_t offset, size_t alignment)
	{
		return offset == alignDown(offset, alignment);
	}

	static constexpr TypeLayout alignUp(const TypeLayout& layout)
	{
		return { alignUp(layout.size, layout.alignment), layout.alignment };
	}
};


struct PlaceTypeOffset
{
	const TypeId* typeId;
	std::unique_ptr<ValueHolder> value;
	size_t offset;
};

using PlaceTypeOffsets = std::vector<PlaceTypeOffset>;

template <typename Alloc = MemoryAllocator>
class MemoryBlock
{
public:
	MemoryBlock()
	{}

	MemoryBlock(const TypeLayout& blockLayout, const PlaceTypeOffsets& placeOffsets)
		: memory(Alloc::allocate(Alloc::alignUp(blockLayout)))
	{
		values.reserve(placeOffsets.size());
		destructs.reserve(placeOffsets.size());
		for (const PlaceTypeOffset& placeOffset : placeOffsets)
		{
			auto* ptr = memory + placeOffset.offset;
			if (placeOffset.value)
			{
				values.push_back(placeOffset.value->constructRef(ptr));
			}
			else
			{
				auto* value = placeOffset.typeId->construct(ptr);
				values.push_back(value);
				destructs.push_back(value);
			}
		}
	}

	~MemoryBlock()
	{
		for (const auto& destruct : destructs)
			destruct->~ValueHolder();
		if (memory != nullptr)
			Alloc::free(memory);
	}

	MemoryBlock(MemoryBlock&& other) noexcept
	{
		std::swap(memory, other.memory);
		std::swap(values, other.values);
		std::swap(destructs, other.destructs);
	}

	MemoryBlock& operator=(MemoryBlock&& other) noexcept
	{
		assert(memory == nullptr);
		std::swap(memory, other.memory);
		std::swap(values, other.values);
		std::swap(destructs, other.destructs);
		return *this;
	}

	MemoryBlock(const MemoryBlock&) = delete;
	MemoryBlock& operator=(const MemoryBlock&) = delete;

	constexpr ValueHolder& get(const size_t index) const
	{
		return *values[index];
	}

private:
	std::byte* memory { nullptr };
	std::vector<ValueHolder*> values;
	std::vector<ValueHolder*> destructs;
};

class DataBlock
{
public:
	DataBlock(PlaceType type);

	PlaceType getType() const
	{
		return placesType;
	}

	PlaceData addPlaceType(const TypeId& typeId)
	{
		return addPlace({ &typeId });
	}

	template <typename T>
	PlaceData addPlaceValue(T val)
	{
		auto value = std::make_unique<SpecTypeValueHolder<T>>();
		const TypeId& typeId = SpecTypeValueHolder<T>::specTypeId;
		value->set(std::move(val));
		return addPlace({ &typeId, std::move(value) });
	}

	const TypeId* getPlaceType(size_t index) const;

	template <typename Alloc = MemoryAllocator>
	MemoryBlock<Alloc> makeMemoryBlock() const
	{
		return blockLayout.size == 0 ? MemoryBlock<Alloc>{} : MemoryBlock<Alloc>{ blockLayout, placeTypeOffsets };
	}

private:
	PlaceData addPlace(PlaceTypeOffset place);

	PlaceType placesType;
	TypeLayout blockLayout;
	PlaceTypeOffsets placeTypeOffsets;
	std::vector<size_t> placeIndices;
};


class Function;

const size_t placeTypesCount { PlaceData::typeIndex(PlaceType::Void) };

class CPPSCRIPT_API CodeBlock : public DataBlock
{
public:
	CodeBlock();

	using Code = std::vector<std::unique_ptr<Function>>;
	Code operations;
	using Caches = std::array<std::vector<std::vector<PlaceDataCache*>>, placeTypesCount>;
	Caches caches;

	void registerCache(PlaceDataCache& cache);
};


class CPPSCRIPT_API ExecutionContext
{
public:
	void run(CodeBlock& code);
	void run();

	void jump(int offset)
	{
		nextCodeOffset = offset;
	}

	ValueHolder& get(const PlaceData& place) const
	{
		return memoryBlocks[PlaceData::typeIndex(place.argType)].get(place.index);
	}

	void refreshCache(PlaceType startType, PlaceType endType);

private:
	CodeBlock* code{nullptr};
	CodeBlock::Code::const_iterator codePointer;
	const int codeStep{ 1 };
	int nextCodeOffset{ codeStep };

	std::array<MemoryBlock<>, placeTypesCount> memoryBlocks;
};


template <typename T>
struct JumpTableTraits
{
	static constexpr size_t size = 0;
};

struct UnconditionalJump
{};

template <> struct JumpTableTraits<UnconditionalJump>
{
	static constexpr size_t size = 1;

	static constexpr int index(const UnconditionalJump val)
	{
		return 0;
	}
};

template <> struct JumpTableTraits<bool>
{
	static constexpr size_t size = 2;

	static constexpr int index(const bool val)
	{
		return val ? 0 : 1;
	}
};

template <typename T>
class JumpTable
{
public:
	JumpTable(const std::vector<int>& jumps) : jumpTable(jumps)
	{}

	void jump(ExecutionContext& context, const T val) const
	{
		return context.jump(jumpTable[JumpTableTraits<T>::index(val)]);
	}

private:
	std::vector<int> jumpTable;
};

}