#pragma once

#include <CppScript/Definitions.h>
#include <array>
#include <vector>


namespace CppScript
{

enum class MemoryType
{
	Local,
	Module,
	Argument,
	Last
};

constexpr size_t memoryTypeIndex(const MemoryType memType)
{
	return static_cast<size_t>(memType);
}


class CPPSCRIPT_API MemoryPlace
{
public:
	MemoryType memoryType;
	size_t index;
};


class CPPSCRIPT_API CachedMemoryPlace : public MemoryPlace
{
public:
	CachedMemoryPlace(MemoryPlace place) : MemoryPlace(place)
	{}

	void setMemory(void* ptr)
    {
        memoryPtr = ptr;
    }

protected:
	void* memoryPtr;
};


class CPPSCRIPT_API ExecutionContext
{
public:
    void* get(const MemoryPlace& place) const;
	void set(const MemoryPlace& place, void* ptr);

private:
	std::array<std::vector<void*>, memoryTypeIndex(MemoryType::Last)> memoryPointers;
};

}