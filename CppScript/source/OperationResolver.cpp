#include <CppScript/OperationResolver.h>

namespace CppScript
{

bool OperationDescription::validate(const OperationBuildContext& context) const
{
    const bool uncondJump = context.jumps.size() == 1;
    const std::size_t returnArg = returnType != nullptr && uncondJump ? 1 : 0;
    return (uncondJump || context.jumps.size() == jumpCount)
        && context.argumentPlaces.size() >= argumentTypes.size()
        && context.argumentPlaces.size() <= argumentTypes.size() + returnArg;
}


void OperationResolver::addDescriptions(std::initializer_list<OperationDescription> descrs)
{
    descriptions.insert(descriptions.end(), descrs);
}

bool OperationResolver::validate(const OperationBuildContext& context) const
{
    return context.index < descriptions.size() && descriptions[context.index].validate(context);
}

OperationResolver::Result OperationResolver::resolve(OperationBlockResolutionData&& block, TypeFrames& frames) const
{
    OperationBlockBuildContext buildContext;
    Error error{ block.blockId };
    std::vector<const TypeId*> argTypes;
    if (!block.constantValues.empty())
    {
        buildContext.constantValues = std::move(block.constantValues);
        auto& constFrame = getFrame(frames, ValuePlace::Type::Constants);
        constFrame.clear();
        constFrame.reserve(buildContext.constantValues.size());
        for (auto& constValue : buildContext.constantValues)
            constFrame.push_back(&constValue->getTypeId());
    }
    for (auto& operation : block.operations)
    {
        error.location = operation.location;
        error.alternatives.clear();
        argTypes.clear();
        for (const ValuePlace& place : operation.argumentPlaces)
        {
            auto& frame = getFrame(frames, place.placeType);
            if (place.index < frame.size() && frame[place.index] != nullptr)
                argTypes.push_back(frame[place.index]);
            else
            {
                error.message = "unknown type of argument";
                return error;
            }
        }
        for (std::size_t index = 0; index < getDescriptions().size(); ++index)
        {
            const auto& descr = getDescriptions()[index];
            if (operation.operationId == descr.id)
            {
                if (std::equal(argTypes.begin(), argTypes.end(),
                    descr.argumentTypes.begin(), descr.argumentTypes.end(),
                    [](const TypeId* reqType, const TypeId* opArgType)
                    {
                        return opArgType->accepts(*reqType);
                    }) && !(descr.returnType == nullptr && operation.returnPlace))
                {
                    OperationBuildContext opContext{ index, std::move(operation.argumentPlaces),
                        std::move(operation.jumps), operation.location };
                    if (operation.returnPlace)
                    {
                        opContext.argumentPlaces.push_back(*operation.returnPlace);
                        auto& frame = getFrame(frames, operation.returnPlace->placeType);
                        if (operation.returnPlace->index >= frame.size())
                            frame.resize(operation.returnPlace->index + 1, nullptr);
                        frame[operation.returnPlace->index] = descr.returnType;
                    }
                    buildContext.operations.push_back(std::move(opContext));
                }
                else
                {
                    error.alternatives.push_back(&descr);
                }
            }
        }
    }
    buildContext.valueTypes = getFrame(frames, ValuePlace::Type::Local);
    return buildContext;
}

OperationResolver::ResolutionContext OperationResolver::createContextWithConstants(OperationBlockResolutionData& block, TypeFrames& frames) const
{
    return ResolutionContext{frames};
}

bool OperationResolver::resolveOperation(OperationResolutionData&& operation, ResolutionContext& context) const
{
    auto& argTypes = context.argumentTypes;
    argTypes.clear();
    context.errorData.location = operation.location;
    context.errorData.alternatives.clear();
    for (const ValuePlace& place : operation.argumentPlaces)
    {
        auto& frame = getFrame(context.frames, place.placeType);
        if (place.index < frame.size() && frame[place.index] != nullptr)
            argTypes.push_back(frame[place.index]);
        else
        {
            context.errorData.message = "unknown type of argument";
            return false;
        }
    }
    for (std::size_t index = 0; index < getDescriptions().size(); ++index)
    {
        const auto& descr = getDescriptions()[index];
        if (operation.operationId == descr.id)
        {
            if (std::equal(argTypes.begin(), argTypes.end(),
                descr.argumentTypes.begin(), descr.argumentTypes.end(),
                [](const TypeId* reqType, const TypeId* opArgType)
                {
                    return opArgType->accepts(*reqType);
                }) && !(descr.returnType == nullptr && operation.returnPlace))
            {
                OperationBuildContext opContext{ index, std::move(operation.argumentPlaces),
                    std::move(operation.jumps), operation.location };
                if (operation.returnPlace)
                {
                    opContext.argumentPlaces.push_back(*operation.returnPlace);
                    auto& frame = getFrame(context.frames, operation.returnPlace->placeType);
                    if (operation.returnPlace->index >= frame.size())
                        frame.resize(operation.returnPlace->index + 1, nullptr);
                    frame[operation.returnPlace->index] = descr.returnType;
                }
                context.blockContext.operations.push_back(std::move(opContext));
                return true;
            }
            else
            {
                context.errorData.alternatives.push_back(&descr);
            }
        }
    }
    return false;
}

}