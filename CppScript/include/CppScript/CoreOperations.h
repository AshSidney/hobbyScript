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
class AddOperation : public OperationHelper<AddOperation<T>, T, const T&, const T&>
{
public:
    T operator()(const T& left, const T& right) const
    {
        return left + right;
    }

    static constexpr OperationId id{ "+" };
};

template <typename T>
class AddToOperation : public OperationHelper<AddToOperation<T>, void, T&, const T&>
{
public:
    void operator()(T& left, const T& right) const
    {
        left += right;
    }

    static constexpr OperationId id{ "+=" };
};

template <typename T>
class SubtractOperation : public OperationHelper<SubtractOperation<T>, T, const T&, const T&>
{
public:
    T operator()(const T& left, const T& right) const
    {
        return left - right;
    }

    static constexpr OperationId id{ "-" };
};

template <typename T>
class SubtractFromOperation : public OperationHelper<SubtractFromOperation<T>, void, T&, const T&>
{
public:
    void operator()(T& left, const T& right) const
    {
        left -= right;
    }

    static constexpr OperationId id{ "-=" };
};

template <typename T>
class MultiplyOperation : public OperationHelper<MultiplyOperation<T>, T, const T&, const T&>
{
public:
    T operator()(const T& left, const T& right) const
    {
        return left * right;
    }

    static constexpr OperationId id{ "*" };
};

template <typename T>
class MultiplyByOperation : public OperationHelper<MultiplyByOperation<T>, void, T&, const T&>
{
public:
    void operator()(T& left, const T& right) const
    {
        left *= right;
    }

    static constexpr OperationId id{ "*=" };
};

template <typename T>
class DivideOperation : public OperationHelper<DivideOperation<T>, T, const T&, const T&>
{
public:
    T operator()(const T& left, const T& right) const
    {
        return left / right;
    }

    static constexpr OperationId id{ "/" };
};

template <typename T>
class DivideByOperation : public OperationHelper<DivideByOperation<T>, void, T&, const T&>
{
public:
    void operator()(T& left, const T& right) const
    {
        left /= right;
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

template <typename T>
class CompareEqualOperation : public OperationHelper<CompareEqualOperation<T>, bool, const T&, const T&>
{
public:
    bool operator()(const T& left, const T& right) const
    {
        return left == right;
    }

    static constexpr OperationId id{ "==" };
};

template <typename T>
class CompareNotEqualOperation : public OperationHelper<CompareNotEqualOperation<T>, bool, const T&, const T&>
{
public:
    bool operator()(const T& left, const T& right) const
    {
        return left != right;
    }

    static constexpr OperationId id{ "!=" };
};

template <typename T>
class CompareLessOperation : public OperationHelper<CompareLessOperation<T>, bool, const T&, const T&>
{
public:
    bool operator()(const T& left, const T& right) const
    {
        return left < right;
    }

    static constexpr OperationId id{ "<" };
};

template <typename T>
class CompareLessEqualOperation : public OperationHelper<CompareLessEqualOperation<T>, bool, const T&, const T&>
{
public:
    bool operator()(const T& left, const T& right) const
    {
        return left <= right;
    }

    static constexpr OperationId id{ "<=" };
};

template <typename T>
class CompareGreaterOperation : public OperationHelper<CompareGreaterOperation<T>, bool, const T&, const T&>
{
public:
    bool operator()(const T& left, const T& right) const
    {
        return left > right;
    }

    static constexpr OperationId id{ ">" };
};

template <typename T>
class CompareGreaterEqualOperation : public OperationHelper<CompareGreaterEqualOperation<T>, bool, const T&, const T&>
{
public:
    bool operator()(const T& left, const T& right) const
    {
        return left >= right;
    }

    static constexpr OperationId id{ ">=" };
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

template <typename T, typename CONSTRUCT, typename ORDERING>
using NumericTypeOperationBuilder = OperationBuilder<CONSTRUCT, CopyOperation<T>,
    AddOperation<T>, AddToOperation<T>, SubtractOperation<T>, SubtractFromOperation<T>,
    MultiplyOperation<T>, MultiplyByOperation<T>, DivideOperation<T>, DivideByOperation<T>,
    CompareOperation<T, ORDERING>, CompareEqualOperation<T>, CompareNotEqualOperation<T>,
    CompareLessOperation<T>, CompareLessEqualOperation<T>,
    CompareGreaterOperation<T>, CompareGreaterEqualOperation<T>>;

using StdInt = IntValue::StdIntType;

class StdIntConstruct : public OperationHelper<StdIntConstruct, StdInt, const std::string&>
{
public:
    inline StdInt operator()(const std::string& val) const
    {

        return std::stoll(val);
    }

    static constexpr OperationId id{ "stdint", "", true };
};

using CoreOperationBuilder = OperationBuilders<NumericTypeOperationBuilder<IntValue, IntConstruct, std::strong_ordering>,
    NumericTypeOperationBuilder<StdInt, StdIntConstruct, std::strong_ordering>>;


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