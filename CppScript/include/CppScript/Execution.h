#pragma once

#include <CppScript/Definitions.h>
#include <CppScript/ValueHolder.h>
#include <memory>
#include <array>
#include <vector>


namespace CppScript
{

enum class PlaceType
{
	Local,
	Module,
	Argument,
	Void
};


class CPPSCRIPT_API PlaceData
{
public:
	PlaceType argType;
	int offset;

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


class Function;

const size_t placeTypesCount { PlaceData::typeIndex(PlaceType::Void) };

struct CPPSCRIPT_API CodeBlock
{
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

	void jump(int offset)
	{
		nextCodeOffset = offset;
	}

    ValueHolder& get(const PlaceData& place) const
	{
		return *placePointers[PlaceData::typeIndex(place.argType)][place.offset];
	}

	void set(const PlaceData& place, ValueHolder& val);

	void refreshCache(PlaceType startType, PlaceType endType);

private:
	CodeBlock* code{nullptr};
	CodeBlock::Code::const_iterator codePointer;
	const int codeStep{1};
	int nextCodeOffset{codeStep};

	std::array<std::vector<ValueHolder*>, placeTypesCount> placePointers;
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
	JumpTable(const std::vector<int>& jumps) : //tableSize(JumpTableTraits<T>::size),
		jumpTable(jumps)
	{}

	void jump(ExecutionContext& context, const T val) const
	{
		return context.jump(jumpTable[JumpTableTraits<T>::index(val)]);
	}

private:
	std::vector<int> jumpTable;
};

}