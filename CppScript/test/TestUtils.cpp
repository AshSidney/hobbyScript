#include "TestUtils.h"

namespace CppScript
{

    bool operator==(const TypeLayout& left, const TypeLayout& right)
    {
        return left.size == right.size && left.alignment == right.alignment;
    }

    TypeLayout operator+(const TypeLayout& left, const TypeLayout& right)
    {
        TypeLayout result{left};
        return result += right;
    }

    TypeLayout operator*(const TypeLayout& layout, const size_t mult)
    {
        return { layout.size * mult, layout.alignment };
    }

    bool operator==(const PlaceData& left, const PlaceData& right)
    {
        return left.argType == right.argType && left.index == right.index;
    }
    
}
