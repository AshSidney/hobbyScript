#include <CppScript/FunctionDef.h>

namespace CppScript
{

Module::Module(std::string_view name) : name(name)
{}

std::unique_ptr<Function> Module::buildFunction(FunctionContext& context) const
{
    auto foundFuncs = functions.find(context.name);
    if (foundFuncs != functions.cend())
        for (const auto& func : foundFuncs->second)
        {
            auto result = func->buildFunction(context);
            if (result)
                return result;
        }
    return {};
}


}