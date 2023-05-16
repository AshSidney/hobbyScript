#include <CppScript/CoreModule.h>
#include <CppScript/IntValue.h>
#include <CppScript/Execution.h>

namespace CppScript
{

Module createCoreModule()
{
    Module coreModule{"core"};

    auto intConstr = [](const std::string& val) { return IntValue{val}; };
    auto intCopyConstr = [](const IntValue& val) { return IntValue{val}; };
    auto intAssign = [](IntValue& left, const IntValue& right) -> IntValue& { return left = right; };
    auto intAdd = [](IntValue& left, const IntValue& right) -> IntValue& { return left += right; };
    auto intSubtract = [](IntValue& left, const IntValue& right) -> IntValue& { return left -= right; };
    auto intCompare = [](const IntValue& left, const IntValue& right) -> Comparison { return compare(left, right); };

    coreModule.defFunction<decltype(intConstr), IntValue, const std::string&>("int", std::move(intConstr))
        .defFunction<decltype(intCopyConstr), IntValue, const IntValue&>("copy", std::move(intCopyConstr))
        .defFunction<decltype(intAssign), IntValue&, IntValue&, const IntValue&>("=", std::move(intAssign))
        .defFunction<decltype(intAdd), IntValue&, IntValue&, const IntValue&>("+=", std::move(intAdd))
        .defFunction<decltype(intSubtract), IntValue&, IntValue&, const IntValue&>("-=", std::move(intSubtract))
        .defFunction<decltype(intCompare), Comparison, const IntValue&, const IntValue&>("<=>", std::move(intCompare));
    
    return coreModule;
}

}