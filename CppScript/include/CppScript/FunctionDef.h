#pragma once

#include <CppScript/Definitions.h>
#include <CppScript/Execution.h>
#include <memory>
#include <vector>
#include <tuple>
#include <unordered_map>

namespace CppScript
{

enum class FunctionOptions
{
    Default = 0,
    Cache = 1,
    Jump = 2
};

constexpr FunctionOptions operator|(const FunctionOptions left, const FunctionOptions right)
{
    return static_cast<FunctionOptions>(static_cast<size_t>(left) | static_cast<size_t>(right));
}

constexpr FunctionOptions operator&(const FunctionOptions left, const FunctionOptions right)
{
    return static_cast<FunctionOptions>(static_cast<size_t>(left) & static_cast<size_t>(right));
}

constexpr bool contains(const FunctionOptions left, const FunctionOptions right)
{
    return (left & right) != FunctionOptions::Default;
}

class Module;

struct FunctionContext
{
    std::string name;
    FunctionOptions options;
    PlaceData returnPlace;
    std::vector<PlaceData> argPlaces;
    std::vector<int> jumps;
    CodeBlock* currentCode{ nullptr };
    Module* currentModule{ nullptr };

    const TypeId& getDataType(const PlaceData& place) const;
};


template <typename T>
class StraightObjectAccessor : public PlaceData
{
public:
    void init(const PlaceData place, FunctionContext& context)
    {
        argType = place.argType;
        index = place.index;
    }

    using ValueAccess = ValueTraits<T>;
    using ValueRef = ValueAccess::ValueRef;

	ValueRef get(const ExecutionContext& context) const
    {
        return ValueAccess::get(context.get(*this));
    }

    void set(const ExecutionContext& context, T val) const
    {
        ValueAccess::set(context.get(*this), std::forward<T>(val));
    }
};


template <typename T>
class CachedObjectAccessor : public PlaceDataCache
{
public:
    void init(const PlaceData place, FunctionContext& context)
    {
        argType = place.argType;
        index = place.index;
        context.currentCode->registerCache(*this);
    }

    using ValueAccess = ValueTraits<T>;
    using ValueRef = ValueAccess::ValueRef;

	ValueRef get(const ExecutionContext& context) const
    {
        return ValueAccess::get(*valueHolder);
    }

    void set(const ExecutionContext& context, T val) const
    {
        ValueAccess::set(*valueHolder, std::forward<T>(val));
    }
};


class CPPSCRIPT_API Function
{
public:
    virtual ~Function() = default;

    virtual void execute(ExecutionContext& context) const = 0;
};


template <typename F, template<typename> typename O, typename ...A>
class FunctionVoid : public Function
{
public:
    FunctionVoid(F func, FunctionContext& context)
        : function(func)
    {
        init(context, std::index_sequence_for<A...>());
    }

    void execute(ExecutionContext& context) const override
    {
        return executeVoid(context, std::index_sequence_for<A...>());
    }

protected:
    template <size_t ...I>
    void init(FunctionContext& context, std::index_sequence<I...>)
    {
        (std::get<I>(arguments).init(context.argPlaces[I], context), ...);
    }

    template <size_t ...I>
    void executeVoid(ExecutionContext& context, std::index_sequence<I...>) const
    {
        function((std::get<I>(arguments).get(context))...);
    }

    F function;
    std::tuple<O<A>...> arguments;
};


template <typename F, template<typename> typename O, typename R, typename ...A>
class FunctionReturn : public FunctionVoid<F, O, A...>
{
public:
    FunctionReturn(F func, FunctionContext& context)
        : FunctionVoid<F, O, A...>(func, context)
    {
        returnTarget.init(context.returnPlace, context);
    }

    void execute(ExecutionContext& context) const override
    {
        executeRet(context, std::index_sequence_for<A...>());
    }

private:
    using FunctionVoid<F, O, A...>::function;
    using FunctionVoid<F, O, A...>::arguments;

    template <size_t ...I>
    void executeRet(ExecutionContext& context, std::index_sequence<I...>) const
    {
        returnTarget.set(context, function((std::get<I>(arguments).get(context))...));
    }

    O<R> returnTarget;
};


template <typename F, template<typename> typename O, typename R, typename ...A>
class FunctionJump : public FunctionVoid<F, O, A...>
{
public:
    FunctionJump(F func, FunctionContext& context)
        : FunctionVoid<F, O, A...>(func, context), jumpTable(context.jumps)
    {}

    void execute(ExecutionContext& context) const override
    {
        executeJump(context, std::index_sequence_for<A...>());
    }

private:
    using FunctionVoid<F, O, A...>::function;
    using FunctionVoid<F, O, A...>::arguments;

    template <size_t ...I>
    void executeJump(ExecutionContext& context, std::index_sequence<I...>) const
    {
        jumpTable.jump(context, function((std::get<I>(arguments).get(context))...));
    }

    JumpTable<R> jumpTable;
};


class CPPSCRIPT_API FunctionDef
{
public:
    virtual ~FunctionDef() = default;

    virtual std::unique_ptr<Function> buildFunction(FunctionContext& context) const = 0;
};


template <typename F, typename R, typename ...A>
struct FunctionDefReturnArgs : public FunctionDef
{
public:
    FunctionDefReturnArgs(F func) :function(func)
    {}

    std::unique_ptr<Function> buildFunction(FunctionContext& context) const override
    {
        if (!validate(context))
            return {};
        return contains(context.options, FunctionOptions::Cache) ?
            createFunction<CachedObjectAccessor>(context) : createFunction<StraightObjectAccessor>(context);
        /*if (contains(context.options, FunctionOptions::Jump))
            return useCache ? CreateJumpFunction<CachedObjectAccessor>(context)
                : CreateJumpFunction<StraightObjectAccessor>(context);
        if (context.returnPlace.argType != PlaceType::Void)
            return useCache ? CreateReturnFunction<CachedObjectAccessor>(context)
                : CreateReturnFunction<StraightObjectAccessor>(context);
        return useCache ? CreateVoidFunction<CachedObjectAccessor>(context)
            : CreateVoidFunction<StraightObjectAccessor>(context);*/
    }

private:
    bool validate(const FunctionContext& context) const
    {
        return ((context.returnPlace.argType == PlaceType::Void || !std::is_void_v<R>)
            && validateArguments(context, { &SpecTypeValueHolder<A>::typeId ... }, std::index_sequence_for<A...>()));
    }

    template <size_t ...I>
    bool validateArguments(const FunctionContext& context, std::initializer_list<const TypeId*> argTypes, std::index_sequence<I...>) const
    {
        return argTypes.size() == context.argPlaces.size() && ((*std::data(argTypes)[I] == context.getDataType(context.argPlaces[I])) && ...);
    }

    template <template<typename> typename O>
    std::unique_ptr<Function> createFunction(FunctionContext& context) const
    {
        std::unique_ptr<Function> func;
        if (contains(context.options, FunctionOptions::Jump))
        {
            if constexpr(!std::is_void_v<R> && JumpTableTraits<R>::size > 0)
                func = std::make_unique<FunctionJump<F, O, R, A...>>(function, context);
        }
        else if (context.returnPlace.argType != PlaceType::Void)
        {
            if constexpr(!std::is_void_v<R>)
                func = std::make_unique<FunctionReturn<F, O, R, A...>>(function, context);
        }
        else
        {
            func = std::make_unique<FunctionVoid<F, O, A...>>(function, context);
        }
        return func;
    }

    F function;
};


class CPPSCRIPT_API Module : public DataBlock
{
public:
    Module(std::string_view name);

    std::unique_ptr<Function> buildFunction(FunctionContext& context) const;

    template <typename F, typename R, typename ...A>
    Module& defFunction(const std::string& name, F func)
    {
        functions[name].emplace_back(std::make_unique<FunctionDefReturnArgs<F, R, A...>>(std::move(func)));

        return *this;
    }

    template <typename C, typename ...A>
    Module& defConstructor(const std::string& name)
    {
        auto func = [](A ...args) -> C
        {
            return C(args...);
        };
        return defFunction<decltype(func), C, A...>(name, std::move(func));
    }

    template <typename R, typename C, typename ...A>
    Module& defFunction(const std::string& name, R(C::*method)(A...))
    {
        auto func = [method](C& obj, A ...args) -> R
        {
            return (obj.*method)(std::forward<A>(args)...);
        };
        return defFunction<decltype(func), R, C, A...>(name, std::move(func));
    }

    template <typename R, typename C, typename ...A>
    Module& defFunction(const std::string& name, R(C::*method)(A...) const)
    {
        auto func = [method](const C& obj, A ...args) -> R
        {
            return (obj.*method)(std::forward<A>(args)...);
        };
        return defFunction<decltype(func), R, C, A...>(name, std::move(func));
    }

    template <typename R, typename ...A>
    Module& defFunction(const std::string& name, R(*function)(A...))
    {
        auto func = [function](A ...args) -> R
        {
            return (*function)(std::forward<A>(args)...);
        };
        return defFunction<decltype(func), R, A...>(name, std::move(func));
    }

    template <typename V>
    Module& defValue(const std::string& name, V value)
    {
        auto func = [value]() -> V
        {
            return value;
        };
        return defFunction<decltype(func), V>(name, std::move(func));
    }

private:
    std::string name;
    std::unordered_map<std::string, std::vector<std::unique_ptr<FunctionDef>>> functions;
};


}