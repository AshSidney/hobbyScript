#include "ValueMock.h"

namespace CppScriptTest
{

ValueMock::ValueMock() = default;

ValueMock::~ValueMock() noexcept = default;

TypeIdMock::TypeIdMock(const CppScript::TypeId::Layout& layout, const bool isVal)
{
    this->layout = layout;
    if (isVal)
        commonTypeId = &CppScript::ValueCommon<bool>::typeId;
}

TypeIdMock::~TypeIdMock() noexcept = default;

}