#pragma once

#include <CppScript/Definitions.h>
#include <CppScript/Execution.h>
#include <memory>
#include <vector>
#include <tuple>

namespace CppScript
{

template <typename T>
class ObjectAccessor
{
protected:
    using Result = T&;

	Result get(void* ptr) const
    {
        return *reinterpret_cast<T*>(ptr);
    }
};

template <typename T>
class ObjectAccessor<T&>
{
protected:
    using Result = T&;

	Result get(void* ptr) const
    {
        return *reinterpret_cast<T*>(ptr);
    }
};


template <typename T>
class StraightObjectAccessor : public MemoryPlace, public ObjectAccessor<T>
{
public:
	StraightObjectAccessor<T>(MemoryPlace place) : MemoryPlace(place)
    {}

	ObjectAccessor<T>::Result get(const ExecutionContext& context) const
    {
        return ObjectAccessor<T>::get(context.get(*this));
    }
};


enum class FunctionSpec
{
    Default = 0,
    ReturnVoid = 1,
    CacheMemory = 2
};

constexpr FunctionSpec operator|(const FunctionSpec left, const FunctionSpec right)
{
    return static_cast<FunctionSpec>(static_cast<size_t>(left) | static_cast<size_t>(right));
}

constexpr FunctionSpec operator&(const FunctionSpec left, const FunctionSpec right)
{
    return static_cast<FunctionSpec>(static_cast<size_t>(left) & static_cast<size_t>(right));
}

constexpr bool Contains(const FunctionSpec left, const FunctionSpec right)
{
    return (left & right) != FunctionSpec::Default;
}

class FunctionContext
{
public:
    FunctionContext(FunctionSpec spec) : funcSpec(spec)
    {}

    //void registerdCachedAccessor(CacheObjectAccessor& accessor);

    bool mustReturn() const
    {
        return !Contains(funcSpec, FunctionSpec::ReturnVoid);
    }

    bool useCache() const
    {
        return Contains(funcSpec, FunctionSpec::CacheMemory);
    }

private:
    FunctionSpec funcSpec;
};


class CPPSCRIPT_API FunctionBase
{
public:
    virtual ~FunctionBase() = default;

    virtual void execute(const ExecutionContext& context) const = 0;
};


class CPPSCRIPT_API FunctionDefBase
{
public:
    virtual ~FunctionDefBase() = default;

    virtual std::unique_ptr<FunctionBase> createFunction(FunctionContext& context, const std::vector<MemoryPlace>& places) const = 0;
};


template <typename F, template<typename> typename O, typename ...A>
class Function : public FunctionBase
{
public:
    Function(F func, const std::vector<MemoryPlace>& places)
        : function(func), arguments(createArguments(places, std::index_sequence_for<A...>()))
    {}

    void execute(const ExecutionContext& context) const override
    {
        executeImpl(context, std::index_sequence_for<A...>());
    }

private:
    template <size_t ...I>
    void executeImpl(const ExecutionContext& context, std::index_sequence<I...>) const
    {
        function((std::get<I>(arguments).get(context))...);
    }

    template <size_t ...I>
    static std::tuple<O<A>...> createArguments(const std::vector<MemoryPlace>& places, std::index_sequence<I...>)
    {
        return {O<A>{places[I]}...};
    }

    F function;
    std::tuple<O<A>...> arguments;
};


template <typename F, typename ...A>
struct FunctionBody : public FunctionDefBase
{
public:
    FunctionBody(F func) : function(func)
    {}

    std::unique_ptr<FunctionBase> createFunction(FunctionContext& context, const std::vector<MemoryPlace>& places) const override
    {
        return places.size() != sizeof...(A) ? std::unique_ptr<FunctionBase>{}
            : std::make_unique<Function<F, StraightObjectAccessor, A...>>(function, places);
    }

private:
    F function;
};


class FunctionDef : public FunctionDefBase
{
public:
    template<typename R, typename C, typename ...A>
    FunctionDef(R(C::*method)(A...))
    {
        if constexpr(!std::is_void_v<R>)
        {
            auto retFunc = [method](R& ret, C& obj, A ...args)
                {
                    ret = (obj.*method)(std::forward<A>(args)...);
                };
            returnFunction = std::make_unique<FunctionBody<decltype(retFunc), R, C, A...>>(std::move(retFunc));
        }
        auto voidFunc = [method](C& obj, A ...args)
            {
                (obj.*method)(std::forward<A>(args)...);
            };
        voidFunction = std::make_unique<FunctionBody<decltype(voidFunc), C, A...>>(std::move(voidFunc));
    }

    template<typename R, typename C, typename ...A>
    FunctionDef(R(C::*method)(A...) const)
    {
        if constexpr(!std::is_void_v<R>)
        {
            auto retFunc = [method](R& ret, const C& obj, A ...args)
                {
                    ret = (obj.*method)(std::forward<A>(args)...);
                };
            returnFunction = std::make_unique<FunctionBody<decltype(retFunc), R, C, A...>>(std::move(retFunc));
        }
        auto voidFunc = [method](const C& obj, A ...args)
            {
                (obj.*method)(std::forward<A>(args)...);
            };
        voidFunction = std::make_unique<FunctionBody<decltype(voidFunc), C, A...>>(std::move(voidFunc));
    }

    template<typename R, typename ...A>
    FunctionDef(R(*function)(A...))
    {
        if constexpr(!std::is_void_v<R>)
        {
            auto retFunc = [function](R& ret, A ...args)
                {
                    ret = (*function)(std::forward<A>(args)...);
                };
            returnFunction = std::make_unique<FunctionBody<decltype(retFunc), R, A...>>(std::move(retFunc));
        }
        auto voidFunc = [function](A ...args)
            {
                (*function)(std::forward<A>(args)...);
            };
        voidFunction = std::make_unique<FunctionBody<decltype(voidFunc), A...>>(std::move(voidFunc));
    }

    bool hasReturn() const
    {
        return static_cast<bool>(returnFunction);
    }

    std::unique_ptr<FunctionBase> createFunction(FunctionContext& context, const std::vector<MemoryPlace>& places) const override
    {
        return context.mustReturn() ? returnFunction->createFunction(context, places)
            : voidFunction->createFunction(context, places);
    }

private:
    std::unique_ptr<FunctionDefBase> returnFunction;
    std::unique_ptr<FunctionDefBase> voidFunction;
};

/*template <typename O, size_t I, typename A>
struct MemoryPlaceArgument
{
    static O<A> Create(const std::vector<MemoryPlace>& places)
    {
        return {places[I]};
    }
};

template <typename O, typename I, typename ...A>
struct MemoryPlaceArguments;

template <typename O, size_t ...I, typename ...A>
struct MemoryPlaceArguments<O, std::index_sequence<I...>, A...>
    : MemoryPlaceArgument<O, I, A>...
{
    template<size_t ID>
    auto Create()
    {
        return MemoryPlaceArgument<O, ID>::Create(places);
    }

    const std::vector<MemoryPlace>& places;
};


template <typename F>
class FunctionDef
{
public:
    FunctionDef(F func) : function(func)
    {}

    template<typename O, typename ...A>
    std::unique_ptr<FunctionBase> createFunction(const std::vector<MemoryPlace>& places) const
    {
        return createFunctionImpl(MemoryPlaceArguments<O, std::index_sequence_for<A...>, A...>{places});
    }

private:
    template <typename PA, size_t ...I>
    std::unique_ptr<FunctionBase> createFunctionImpl(const PA& placeArguments, std::index_sequence<I...>) const
    {
        return std::make_unique<Function<F, A...>>(function, StraightObjectAccessor<A>(places[I])...);+
    }

    F function;
};*/


/*template <typename R, typename C, typename ...A>
class MethodDef : public FunctionDefBase
{
public:
    using MethodPtr = R(C::*)(A...);
    using ConstMethodPtr = R(C::*)(A...) const;

    MethodDef(MethodPtr m) : method(m)
    {}

    std::unique_ptr<FunctionBase> createFunction(FunctionSpec spec, const std::vector<MemoryPlace>& places) const override
    {
        if constexpr(!std::is_void_v<R>)
            return createFunctionImpl(places, std::make_index_sequence<sizeof...(A)>());
        return createVoidFunctionImpl(places, std::make_index_sequence<sizeof...(A)>());
    }

private:
    template <size_t ...I>
    std::unique_ptr<FunctionBase> createFunctionImpl(const std::vector<MemoryPlace>& places, std::index_sequence<I...>) const
    {
        auto* function = new Function([method=method](R& ret, C& obj, A ...args)
        {
            ret = (obj.*method)(args...);
        }, StraightObjectAccessor<R>(places[0]), StraightObjectAccessor<C>(places[1]), StraightObjectAccessor<A>(places[I + 2])...);
        return std::unique_ptr<FunctionBase>(function);
    }

    template <size_t ...I>
    std::unique_ptr<FunctionBase> createVoidFunctionImpl(const std::vector<MemoryPlace>& places, std::index_sequence<I...>) const
    {
        auto* function = new Function([method=method](C& obj, A ...args)
        {
            (obj.*method)(args...);
        }, StraightObjectAccessor<C>(places[0]), StraightObjectAccessor<A>(places[I + 1])...);
        return std::unique_ptr<FunctionBase>(function);
    }

    MethodPtr method;
};


template <typename R, typename ...A>
class FunctionDef : public FunctionDefBase
{
public:
    using FunctionPtr = R(*)(A...);

    FunctionDef(FunctionPtr f) : function(f)
    {}

    std::unique_ptr<FunctionBase> createFunction(FunctionSpec spec, const std::vector<MemoryPlace>& places) const override
    {
        return createFunctionImpl(places, std::make_index_sequence<sizeof...(A)>());
    }

private:
    template <size_t ...I>
    std::unique_ptr<FunctionBase> createFunctionImpl(const std::vector<MemoryPlace>& places, std::index_sequence<I...>) const
    {
        auto* funct = new Function([function=function](R& ret, A ...args)
        {
            ret = (*function)(args...);
        }, StraightObjectAccessor<R>(places[0]), StraightObjectAccessor<A>(places[I + 1])...);
        return std::unique_ptr<FunctionBase>(funct);
    }

    FunctionPtr function;
};*/





/*enum class Specifier
{
    None = 0,
    Const = 1
};

constexpr bool operator==(const Specifier left, const Specifier right)
{
    return static_cast<unsigned int>(left) == static_cast<unsigned int>(right);
}

constexpr Specifier operator|(const Specifier left, const Specifier right)
{
    return static_cast<Specifier>(static_cast<unsigned int>(left) | static_cast<unsigned int>(right));
}


template <class T, typename R, typename ...A, Specifier ...spec>
struct MethodSignature
template <class T, typename R, typename ...A>
class MethodDef
{
public:
	using MethodType = R(T::*)(A ...args);

	MethodDef(MethodType methPtr, ObjectAccessor<T> thisAccess, ObjectAccessor<R> retAccess, ObjectAccessor<A> ...argAccess)
        : method(methPtr), thisAccessor(thisAccess), returnAccessor(retAccess), argAccessor(argAccess...)
    {}

    void operator()()
    {
        returnAccessor.get() = (thisAccessor.get().*method)();
    }

    MemoryAccessor& getThis()
    {
        return thisAccessor;
    }

    MemoryAccessor& getReturn()
    {
        return returnAccessor;
    }

    template <size_t I>
    MemoryAccessor& getArgument()
    {
        return std::get<I>(argAccessor);
    }

private:
    MethodType method;
    ObjectAccessor<T> thisAccessor;
    ObjectAccessor<R> returnAccessor;
    std::tuple<ObjectAccessor<A>...> argAccessor;
};*/

}