#pragma once

#include <CppScript/Definitions.h>
#include <CppScript/Operation.h>
#include <CppScript/Value.h>
#include <CppScript/IntValue.h>

namespace CppScript
{

template <typename T>
class CopyOperation : public OperationHelper<CopyOperation<T>, T, const T&>
{
public:
    T operator()(const T& source) const
    {
        return source;
    }

    static constexpr OperationId id{ "=" };
};

template <typename T>
class AddOperation : public OperationHelper<AddOperation<T>, void, T&, const T&>
{
public:
    void operator()(T& target, const T& source) const
    {
        target += source;
    }

    static constexpr OperationId id{ "+=" };
};

template <typename T>
class SubtractOperation : public OperationHelper<SubtractOperation<T>, void, T&, const T&>
{
public:
    void operator()(T& target, const T& source) const
    {
        target -= source;
    }

    static constexpr OperationId id{ "-=" };
};

template <typename T>
class MultiplyOperation : public OperationHelper<MultiplyOperation<T>, void, T&, const T&>
{
public:
    void operator()(T& target, const T& source) const
    {
        target *= source;
    }

    static constexpr OperationId id{ "*=" };
};

template <typename T>
class DivideOperation : public OperationHelper<DivideOperation<T>, void, T&, const T&>
{
public:
    void operator()(T& target, const T& source) const
    {
        target /= source;
    }

    static constexpr OperationId id{ "/=" };
};

template <typename T, typename O>
class CompareOperation : public OperationHelper<CompareOperation<T, O>, O, const T&, const T&>
{
public:
    O operator()(const T& left, const T& right) const
    {
        return left <=> right;
    }

    static constexpr OperationId id{ "<=>" };
};


class IntConstruct : public OperationHelper<IntConstruct, IntValue, const std::string&>
{
public:
    inline IntValue operator()(const std::string& val) const
    {
        return IntValue{ val };
    }

    static constexpr OperationId id{ "int", "", true };
};


using CoreOperationBuilder = OperationBuilder<IntConstruct, CopyOperation<IntValue>, AddOperation<IntValue>, SubtractOperation<IntValue>,
    MultiplyOperation<IntValue>, DivideOperation<IntValue>, CompareOperation<IntValue, std::strong_ordering>,
    CopyOperation<IntValue::StdIntType>, AddOperation<IntValue::StdIntType>, SubtractOperation<IntValue::StdIntType>,
    MultiplyOperation<IntValue::StdIntType>, DivideOperation<IntValue::StdIntType>, CompareOperation<IntValue::StdIntType, std::strong_ordering>>;


    
using StdInt = IntValue::StdIntType;


using IntConstructL = LambdaOperationHelper<[](){ return OperationId{"int", "", true}; },
    [](const std::string& val){ return IntValue{ val }; },
    IntValue, const std::string&>;

using IntCopyL = LambdaOperationHelper<[](){ return OperationId{"="}; },
    [](const IntValue& val){ return val; },
    IntValue, const IntValue&>;

using IntAddL = LambdaOperationHelper<[](){ return OperationId{"+="}; },
    [](IntValue& result, const IntValue& val){ result += val; },
    void, IntValue&, const IntValue&>;

using IntSubtractL = LambdaOperationHelper<[](){ return OperationId{"-="}; },
    [](IntValue& result, const IntValue& val){ result -= val; },
    void, IntValue&, const IntValue&>;

using IntCompareL = LambdaOperationHelper<[](){ return OperationId{"<=>"}; },
    [](const IntValue& left, const IntValue& right){ return left <=> right; },
    std::strong_ordering, const IntValue&, const IntValue&>;

using StdIntCopyL = LambdaOperationHelper<[](){ return OperationId{"="}; },
    [](const StdInt val){ return val; },
    StdInt, const StdInt>;

using StdIntAddL = LambdaOperationHelper<[](){ return OperationId{"+="}; },
    [](StdInt& result, const StdInt val){ result += val; },
    void, StdInt&, const StdInt>;

using StdIntSubtractL = LambdaOperationHelper<[](){ return OperationId{"-="}; },
    [](StdInt& result, const StdInt val){ result -= val; },
    void, StdInt&, const StdInt>;

using StdIntCompareL = LambdaOperationHelper<[](){ return OperationId{"<=>"}; },
    [](const StdInt left, const StdInt right){ return left <=> right; },
    std::strong_ordering, const StdInt, const StdInt>;

using CoreOperationLBuilder = OperationBuilder<IntConstructL, IntCopyL, IntAddL, IntSubtractL, IntCompareL,
    StdIntCopyL, StdIntAddL, StdIntSubtractL, StdIntCompareL>;


using CoreOperationVBuilder = OperationVBuilder<IntConstruct, CopyOperation<IntValue>, AddOperation<IntValue>,
    SubtractOperation<IntValue>, CompareOperation<IntValue, std::strong_ordering>,
    CopyOperation<StdInt>, AddOperation<StdInt>, SubtractOperation<StdInt>, CompareOperation<StdInt, std::strong_ordering>>;

}