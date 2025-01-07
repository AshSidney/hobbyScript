#pragma once

#include <CppScript/ValueHolder.h>
#include <CppScript/Execution.h>

namespace CppScript
{

    bool operator==(const TypeLayout& left, const TypeLayout& right);
    TypeLayout operator*(const TypeLayout& layout, size_t mult);

    bool operator==(const PlaceData& left, const PlaceData& right);


    template <typename T>
    std::unique_ptr<SpecTypeValueHolder<T>> makeValue(T val)
    {
        auto holder = std::make_unique<SpecTypeValueHolder<T>>();
        holder->setVal(val);
        return holder;
    }

}
