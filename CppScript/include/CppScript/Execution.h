#pragma once

#include <CppScript/Definitions.h>
#include <CppScript/ValueHolder.h>
#include <memory>
#include <span>
#include <array>
#include <vector>
#include <list>
#include <variant>
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
		size_t alignedOffset = alignDown(offset, alignment);
		if (offset != alignedOffset)
			alignedOffset += alignment;
		return alignedOffset;
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


class CPPSCRIPT_API DataTypePlace
{
public:
	DataTypePlace(const TypeId& typeId, const size_t offset, const ValueHolder* source)
		: typeId(&typeId), offset(offset), source(source)
	{}

	const TypeId& getTypeId() const
	{
		return *typeId;
	}

	void setArgument(const ValueHolder& arg) const
	{
		source = &arg;
	}

	struct CPPSCRIPT_API Value
	{
		ValueHolder* holder{ nullptr };
		bool mustDestruct{ false };
	};

	Value makeValue(std::byte* buffer) const
	{
		const auto ptr = buffer + offset;
		assert(typeId->isReference == (source != nullptr));
		return { source == nullptr ? typeId->construct(ptr) : typeId->constructRef(ptr, *source), !typeId->isReference };
	}

private:
	const TypeId* typeId{ nullptr };
	size_t offset{ 0 };
	mutable const ValueHolder* source{ nullptr };
};


struct CPPSCRIPT_API DataBlockDef
{
	struct Layout
	{
		TypeLayout blockLayout;
		std::vector<DataTypePlace> values;
		size_t valuesOffset { 0 };
		size_t destructCount { 0 };
	};
	Layout layout;

	using Values = std::vector<std::unique_ptr<ValueHolder>>;
	Values sourceValues;

	class CPPSCRIPT_API Builder
	{
	public:
		DataBlockDef build();
		size_t addPlace(const TypeId& typeId);
		size_t addPlace(std::unique_ptr<ValueHolder> value);

	private:
		size_t addPlace(const TypeId& typeId, const ValueHolder* value);

		void clear();
		
		struct TypePlaceOffset
		{
			const TypeId* typeId{ nullptr };
			size_t offset{ 0 };
			const ValueHolder* source{ nullptr };
		};

		TypeLayout blockLayout;
		std::vector<TypePlaceOffset> places;
		Values values;
		std::vector<size_t> placeIndices;
		size_t destructCount { 0 };
	};
};


template <typename Alloc = MemoryAllocator>
class DataBlock
{
public:
	DataBlock() = default;

	explicit DataBlock(const DataBlockDef::Layout& layout)
		: memory(layout.blockLayout.size > 0 ? Alloc::allocate(layout.blockLayout) : nullptr),
		values(reinterpret_cast<ValueHolder**>(memory + layout.valuesOffset), layout.values.size()),
		destructibles(values.endPtr(), layout.destructCount)
	{
		auto valueIt = values.begin();
		auto destrIt = destructibles.begin();
		for (auto& value : layout.values)
		{
			auto newValue = value.makeValue(memory);
			*valueIt = newValue.holder;
			if (newValue.mustDestruct)
			{
				*destrIt = *valueIt;
				++destrIt;
			}
			++valueIt;
		}
		assert(valueIt == values.end());
		assert(destrIt == destructibles.end());
	}

	~DataBlock()
	{
		for (auto* destr : destructibles)
			destr->~ValueHolder();
		if (memory != nullptr)
			Alloc::free(memory);
	}

	DataBlock(DataBlock&& other) noexcept
	{
		std::swap(memory, other.memory);
		std::swap(values, other.values);
		std::swap(destructibles, other.destructibles);
	}

	DataBlock& operator=(DataBlock&& other) noexcept
	{
		memory = other.memory;
		values = other.values;
		destructibles = other.destructibles;
		other.memory = nullptr;
		other.values = { nullptr, 0 };
		other.destructibles = { nullptr, 0 };
		return *this;
	}

	constexpr ValueHolder& get(const size_t index) const
	{
		return *values.get(index);
	}

private:
	class HolderList
	{
	public:
		using HolderSpan = std::span<ValueHolder*>;

		constexpr HolderList() = default;

		constexpr HolderList(ValueHolder** first, const size_t count)
			: holders{ count > 0 ? HolderSpan{ first, count } : HolderSpan{} }
		{
			assert(first != nullptr || count == 0);
		}

		constexpr ValueHolder*& get(const size_t index) const
		{
			assert(index < holders.size());
			return holders[index];
		}

		constexpr HolderSpan::iterator begin()
		{
			return holders.begin();
		}

		constexpr HolderSpan::iterator end()
		{
			return holders.end();
		}

		constexpr ValueHolder** endPtr() const
		{
			return holders.data() + holders.size();
		}
		
	private:
		HolderSpan holders;
	};

	std::byte* memory { nullptr };
	HolderList values;
	HolderList destructibles;
};




struct PlaceTypeOffset
{
	const TypeId* typeId;
	std::unique_ptr<ValueHolder> value;
	size_t offset;
};

using PlaceTypeOffsets = std::vector<PlaceTypeOffset>;

template <typename Alloc = MemoryAllocator>
class MemoryBlockOld
{
public:
	MemoryBlockOld()
	{}

	MemoryBlockOld(const TypeLayout& blockLayout, const PlaceTypeOffsets& placeOffsets)
		: memory(Alloc::allocate(Alloc::alignUp(blockLayout)))
	{
		values.reserve(placeOffsets.size());
		destructs.reserve(placeOffsets.size());
		for (const PlaceTypeOffset& placeOffset : placeOffsets)
		{
			auto* ptr = memory + placeOffset.offset;
			if (placeOffset.value)
			{
				values.push_back(placeOffset.typeId->constructRef(ptr, *placeOffset.value));
			}
			else
			{
				auto* value = placeOffset.typeId->construct(ptr);
				values.push_back(value);
				destructs.push_back(value);
			}
		}
	}

	~MemoryBlockOld()
	{
		for (const auto& destruct : destructs)
			destruct->~ValueHolder();
		if (memory != nullptr)
			Alloc::free(memory);
	}

	MemoryBlockOld(MemoryBlockOld&& other) noexcept
	{
		std::swap(memory, other.memory);
		std::swap(values, other.values);
		std::swap(destructs, other.destructs);
	}

	MemoryBlockOld& operator=(MemoryBlockOld&& other) noexcept
	{
		assert(memory == nullptr);
		std::swap(memory, other.memory);
		std::swap(values, other.values);
		std::swap(destructs, other.destructs);
		return *this;
	}

	MemoryBlockOld(const MemoryBlockOld&) = delete;
	MemoryBlockOld& operator=(const MemoryBlockOld&) = delete;

	constexpr ValueHolder& get(const size_t index) const
	{
		return *values[index];
	}

private:
	std::byte* memory { nullptr };
	std::vector<ValueHolder*> values;
	std::vector<ValueHolder*> destructs;
};

class DataBlockOld
{
public:
	DataBlockOld(PlaceType type);

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
		const TypeId& typeId = SpecTypeValueHolder<T&>::specTypeId;
		value->setVal(std::move(val));
		return addPlace({ &typeId, std::move(value) });
	}

	const TypeId* getPlaceType(size_t index) const;

	template <typename Alloc = MemoryAllocator>
	MemoryBlockOld<Alloc> makeMemoryBlock() const
	{
		return blockLayout.size == 0 ? MemoryBlockOld<Alloc>{} : MemoryBlockOld<Alloc>{ blockLayout, placeTypeOffsets };
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


class CPPSCRIPT_API CodeBlock
{
public:
	explicit CodeBlock(DataBlockDef data);

	using Code = std::vector<std::unique_ptr<Function>>;
	Code operations;
	using Caches = std::array<std::vector<std::vector<PlaceDataCache*>>, placeTypesCount>;
	Caches caches;

	const DataBlockDef::Layout& getDataLayout() const
	{
		return dataDef.layout;
	}

	void registerCache(PlaceDataCache& cache);

private:
	DataBlockDef dataDef;
};


class ScriptFunction;
class ScriptModule;

class CPPSCRIPT_API ExecutionContext
{
public:
	void run(const CodeBlock& code, DataBlock<>& data);
	void run(const ScriptFunction& func);
	void run(const ScriptModule& module);

	void jump(int offset)
	{
		currentFrame->nextCodeOffset = offset;
	}

	ValueHolder& get(const PlaceData& place) const
	{
		return currentData[PlaceData::typeIndex(place.argType)]->get(place.index);
	}

	void refreshCache(PlaceType placeType);

private:
	static const int codeStep{ 1 };

	struct CodeFrame
	{
		DataBlock<> localData;
		DataBlock<>* moduleData;
		const CodeBlock* code;
		CodeBlock::Code::const_iterator codePointer;
		int nextCodeOffset{ codeStep };

		void run(ExecutionContext& context);
	};

	std::list<CodeFrame> codeStack;

	std::list<DataBlock<>> moduleData;

	CodeFrame* currentFrame{ nullptr };
	std::array<DataBlock<>*, placeTypesCount> currentData;
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