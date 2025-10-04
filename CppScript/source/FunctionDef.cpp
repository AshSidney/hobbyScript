#include <CppScript/FunctionDef.h>

namespace CppScriptOld
{

const TypeIdOld& FunctionContext::getDataType(const PlaceData& place) const
{
    const TypeIdOld* foundType{ &noTypeId };
    if (place.argType == PlaceType::Local && currentCode != nullptr)
    {
        foundType = &currentCode->getDataLayout().values[place.index].getTypeId();
    }
    else if (place.argType == PlaceType::Module && moduleLayout != nullptr)
    {
        foundType = &moduleLayout->values[place.index].getTypeId();
    }
    return *foundType;
}


Module::Module(std::string_view name, const DataBlockDef::Layout& layout) : DataBlock<>(layout), name(name)
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