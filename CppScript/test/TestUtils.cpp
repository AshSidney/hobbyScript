#include "TestUtils.h"
#include <gmock/gmock.h>

using namespace CppScript;

namespace CppScriptTest
{

}


namespace CppScriptOld
{

// deprecated
bool operator==(const TypeLayout& left, const TypeLayout& right)
{
    return left.size == right.size && left.alignment == right.alignment;
}

TypeLayout operator*(const TypeLayout& layout, const size_t mult)
{
    assert(mult > 0);
    return { layout.size * mult, layout.alignment };
}

bool operator==(const PlaceData& left, const PlaceData& right)
{
    return left.argType == right.argType && left.index == right.index;
}
    
}
