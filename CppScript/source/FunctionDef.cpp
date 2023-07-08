#include <CppScript/FunctionDef.h>

namespace CppScript
{

const TypeId& FunctionContext::getDataType(const PlaceData& place) const
{
    static TypeId noTypeId;
    const TypeId* foundType{ &noTypeId };
    if (place.argType < PlaceType::Module && currentBlock != nullptr)
    {
        if (place.offset < currentBlock->dataTypes.size())
            foundType = currentBlock->dataTypes[place.offset];
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