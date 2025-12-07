#include "OperationResolverProxy.h"

namespace CppScriptTest
{

OperationResolverProxy::OperationResolverProxy(const CppScript::OperationResolver& resolver)
{
    for (const auto& descr : resolver.getDescriptions())
        addDescriptions({descr});
}

OperationResolverProxy::ResolutionContext OperationResolverProxy::createContextWithConstants(CppScript::OperationBlockResolutionData& block, CppScript::TypeFrames& frames) const
{
    return CppScript::OperationResolver::createContextWithConstants(block, frames);
}

bool OperationResolverProxy::resolveOperation(CppScript::OperationResolutionData operation, ResolutionContext& context) const
{
    return CppScript::OperationResolver::resolveOperation(std::move(operation), context);
}

}