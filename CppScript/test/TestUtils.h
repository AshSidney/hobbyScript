#pragma once

#include <CppScript/ValueHolder.h>
#include <CppScript/Execution.h>

namespace CppScript
{

    bool operator==(const TypeLayout& left, const TypeLayout& right);
    TypeLayout operator+(const TypeLayout& left, const TypeLayout& right);
    TypeLayout operator*(const TypeLayout& layout, size_t mult);

    bool operator==(const PlaceData& left, const PlaceData& right);

}

template <typename T>
CppScript::TypeLayout makeTypeLayout()
{
    return { sizeof(T), alignof(T) };
}
