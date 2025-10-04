#include "TestUtils.h"
#include <gmock/gmock.h>

using namespace CppScript;

namespace CppScriptTest
{

OperationResolver::Resolved getOpRes(const OperationResolver& resolver, const Id& id, const std::vector<const TypeId*>& argTypes)
{
	const auto result = resolver.resolve(id, argTypes);
    EXPECT_TRUE(std::holds_alternative<OperationResolver::Resolved>(result));
	return std::get<OperationResolver::Resolved>(result);
}

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
