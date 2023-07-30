#include <CppScript/FunctionDef.h>

namespace CppScript
{

const TypeId& FunctionContext::getDataType(const PlaceData& place) const
{
    struct NoType{};
    static ValueTypeId<NoType> noTypeId;
    const TypeId* foundType{ &noTypeId };
    if (place.argType == PlaceType::Local && currentCode != nullptr)
    {
        if (const TypeId* localType = currentCode->getPlaceType(place.index); localType != nullptr)
            foundType = localType;
    }
    return *foundType;
}


Module::Module(std::string_view name) : DataBlock(PlaceType::Module), name(name)
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