#include <CppScript/OperationResolver.h>

namespace CppScript
{

bool OperationDescription::validate(const OperationBuildContext& context) const
{
    const bool noCondJump = context.jumps.size() <= 1;
    const std::size_t returnArg = returnType != nullptr && noCondJump ? 1 : 0;
    return (noCondJump || context.jumps.size() == jumpCount) && argumentTypes.size() + returnArg == context.argumentPlaces.size();
}


OperationResolver::OperationResolver(std::vector<OperationDescription> descrs)
{
    for (std::size_t index = 0; index < descrs.size(); ++index)
        descriptions.push_back({ index, std::move(descrs[index]) });
}

OperationResolver::Result OperationResolver::resolve(const Id& id, const std::vector<const TypeId*>& argTypes) const
{
    auto found = std::find_if(descriptions.begin(), descriptions.end(), [&id, &argTypes](const Description& descr)
    {
        return id == descr.opDescription.id && std::equal(argTypes.begin(), argTypes.end(),
            descr.opDescription.argumentTypes.begin(), descr.opDescription.argumentTypes.end(),
            [](const TypeId* reqType, const TypeId* opArgType)
            {
                assert(reqType != nullptr);
                return opArgType->accepts(*reqType);
            });
    });
    if (found != descriptions.end())
        return Resolved{ found->index, found->opDescription };
    return Result{};
}

void OperationResolver::setModule(std::string_view modId)
{
    
}

void OperationResolver::addDescription(OperationDescription descr)
{
    descriptions.push_back({ descriptions.size(), std::move(descr) });
}

std::size_t OperationResolver::getCount() const
{
    return descriptions.size();
}

bool OperationResolver::validate(const OperationBuildContext& context) const
{
    const auto foundDescr = std::find_if(descriptions.begin(), descriptions.end(), [&context](const Description& descr)
        {
            return descr.index == context.index;
        });
    return foundDescr != descriptions.end() && foundDescr->opDescription.validate(context);
}

}