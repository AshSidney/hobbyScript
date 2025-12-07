#pragma once

#include <CppScript/OperationResolver.h>

namespace CppScriptTest
{

class OperationResolverProxy : public CppScript::OperationResolver
{
public:
    explicit OperationResolverProxy(const CppScript::OperationResolver& resolver);

    using ResolutionContext = CppScript::OperationResolver::ResolutionContext;

    ResolutionContext createContextWithConstants(CppScript::OperationBlockResolutionData& block, CppScript::TypeFrames& frames) const;

    bool resolveOperation(CppScript::OperationResolutionData operation, ResolutionContext& context) const;
};

}