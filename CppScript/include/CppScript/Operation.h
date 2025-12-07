#pragma once

#include <CppScript/Definitions.h>
#include <CppScript/Value.h>
#include <CppScript/EnumTraits.h>
#include <CppScript/OperationResolver.h>
#include <variant>
#include <tuple>
#include <type_traits>
#include <vector>
#include <memory>
#include <string_view>
#include <utility>
#include <span>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <cassert>

namespace CppScript
{

using Arguments = ValueBase* const *;

struct OperationContext
{
	Arguments arguments;
	int nextStep{ 0 };
};


class CPPSCRIPT_API CustomOperation
{
public:
	class Base
	{
	public:
		virtual ~Base() noexcept = default;
		virtual void execute(OperationContext& context) const = 0;
	};

	explicit CustomOperation(std::unique_ptr<Base> op) : operation(std::move(op))
	{
		assert(operation);
	}

	void execute(OperationContext& context) const
	{
		operation->execute(context);
	}

protected:
	std::unique_ptr<Base> operation;
};


template <typename O>
class Operation
{
public:
	using OperationVariant = O;

	Operation(OperationVariant&& op, OperationBuildContext&& context)
		: operation(std::move(op)),
		argumentPlaces(std::move(context.argumentPlaces)),
		location(std::move(context.location))
	{}

	void execute(OperationContext& context) const
	{
		std::visit([&context](const auto& op){ op.execute(context); }, operation);
	}
	
	const std::vector<ValuePlace>& getArgumentPlaces() const
	{
		return argumentPlaces;
	}

	const OperationLocation& getLocation() const
	{
		return location;
	}

private:
	OperationVariant operation;
	std::vector<ValuePlace> argumentPlaces;
	OperationLocation location;
};


template <typename O>
std::vector<const std::vector<ValuePlace>*> getArgumentPlaces(const std::vector<O>& operations)
{
	std::vector<const std::vector<ValuePlace>*> argPlaces;
	argPlaces.reserve(operations.size());
	std::transform(operations.cbegin(), operations.cend(), std::back_inserter(argPlaces),
		[](const O& operation)
		{
			return &operation.getArgumentPlaces();
		});
	return argPlaces;
}


template <typename O>
class ExecutorCommon
{
public:
	explicit ExecutorCommon(OperationBuildContext& context)
	{
		operation.initialize(context);
		assert(context.jumps.size() == 1);
		nextStep = context.jumps.front();
	}
	
protected:
	O operation;
	int nextStep{ 1 };
};

template <typename O>
class ExecutorVoid : public ExecutorCommon<O>
{
public:
	using Base = ExecutorCommon<O>;

	explicit ExecutorVoid(OperationBuildContext& context) : Base(context)
	{}
	
	void execute(OperationContext& context) const
	{
		Base::operation.executeVoid(context);
		context.nextStep = Base::nextStep;
	}
};

template <typename O>
class ExecutorRet : public ExecutorCommon<O>
{
public:
	using Base = ExecutorCommon<O>;

	explicit ExecutorRet(OperationBuildContext& context) : Base(context)
	{}
	
	void execute(OperationContext& context) const
	{
		Base::operation.executeRet(context);
		context.nextStep = Base::nextStep;
	}
};

template <typename O>
class ExecutorJump
{
public:
	explicit ExecutorJump(OperationBuildContext& context)
	{
		operation.initialize(context);
		assert(context.jumps.size() == O::jumpSize);
		std::copy(context.jumps.begin(), context.jumps.end(), jumpTable.begin());
	}
	
	void execute(OperationContext& context) const
	{
		context.nextStep = jumpTable[operation.executeJump(context)];
	}
	
private:
	O operation;
	std::array<int, O::jumpSize> jumpTable;
};


template <typename O>
using Executors = std::conditional_t<O::voidReturn,
		std::variant<ExecutorVoid<O>>,
	std::conditional_t<O::jumpSize == 1,
		std::variant<ExecutorVoid<O>, ExecutorRet<O>>,
		std::variant<ExecutorVoid<O>, ExecutorRet<O>, ExecutorJump<O>>>>;


template <typename OV, typename O>
OV createOperationExecutor(OperationBuildContext& context)
{
	if constexpr (O::jumpSize > 1)
	{
		if (context.jumps.size() > 1)
			return ExecutorJump<O>{ context };
	}
	if constexpr (!O::voidReturn)
	{
		if (context.argumentPlaces.size() > O::argumentCount)
			return ExecutorRet<O>{ context };
	}
	return ExecutorVoid<O>{ context };
}


template <typename ... E>
struct OperationExecutorsImpl;

template <typename ... O>
using OperationExecutors = typename OperationExecutorsImpl<std::variant<CustomOperation>, O...>::Type;

template <typename ... E>
struct OperationExecutorsImpl<std::variant<E...>>
{
	using Type = std::variant<E...>;
};

template <typename ... E, typename ... Es>
struct OperationExecutorsImpl<std::variant<E...>, std::variant<Es...>>
{
	using Type = std::variant<E..., Es...>;
};

template <typename ... E, typename O, typename ... Os>
struct OperationExecutorsImpl<std::variant<E...>, O, Os...>
{
	using Type = typename OperationExecutorsImpl<typename OperationExecutorsImpl<std::variant<E...>, Executors<O>>::Type, Os...>::Type;
};


template <typename ... O>
class OperationBuilder
{
public:
	using OperationType = Operation<OperationExecutors<O...>>;

	OperationBuilder()
	{
		resolver.addDescriptions({ O::getDescription() ... });
	}

	OperationType build(OperationBuildContext&& context) const
	{
		assert(context.index < builders.size());
		assert(resolver.validate(context));
		auto op { builders[context.index](context) };
		return { std::move(op), std::move(context) };
	}

	std::vector<OperationType> build(std::vector<OperationBuildContext> contexts) const
	{
		std::vector<OperationType> operations;
		operations.reserve(contexts.size());
		for (OperationBuildContext& context : contexts)
			operations.push_back(build(std::move(context)));
		return operations;
	}

	const OperationResolver& getResolver() const
	{
		return resolver;
	}

	template <typename ... CO>
	void addCustomOperations()
	{
		(addCustomOperation<CO>(), ...);
		resolver.addDescriptions({ CO::getDescription() ... });
	}

protected:
	using OperationVariant = OperationType::OperationVariant;
	using Builder = OperationVariant(OperationBuildContext&);

	static constexpr std::vector<Builder*> createBuilders()
	{
		std::vector<Builder*> opBuilders;
		( opBuilders.push_back([](OperationBuildContext& context)
			{
				return createOperationExecutor<OperationVariant, O>(context);
			}), ...);
		return opBuilders;
	}

	template <typename E>
	class CustomOperationExecutor : public CustomOperation::Base
	{
	public:
		CustomOperationExecutor(E exec) : executor(std::move(exec))
		{}

		void execute(OperationContext& context) const override
		{
			executor.execute(context);
		}

	private:
		E executor;
	};

	template <typename CO>
	void addCustomOperation()
	{
		builders.push_back([](OperationBuildContext& context)
		{
			auto customOp{ std::visit([](auto&& exec)
				{
					return CustomOperation{ std::make_unique<CustomOperationExecutor<std::decay_t<decltype(exec)>>>(std::move(exec)) };
				},
				createOperationExecutor<Executors<CO>, CO>(context)) };

			return OperationVariant{ CustomOperation{ std::move(customOp) } };
		});
	}
	
	std::vector<Builder*> builders{ createBuilders() };

	OperationResolver resolver;
};


template <typename ... O>
struct OperationBuilderExtension;

template <typename ... O, typename ... E>
struct OperationBuilderExtension<OperationBuilder<O...>, E...>
{
	using Type = OperationBuilder<O..., E...>;
};


template <typename T, typename V = void>
struct JumpTraits
{
public:
	static constexpr std::size_t size{ 1 };
};

template <typename E> 
struct JumpTraits<E, typename std::enable_if_t<enumDefined<E>>>
{
public:
	static constexpr std::size_t size{ EnumTraits<E>::size };

	static constexpr std::size_t index(const E value)
	{
		return EnumTraits<E>::index(value);
	}
};


struct OperationId
{
	Id operationId;
	bool isConstructor{ false };
};


template <typename O, typename R, typename ... A>
class OperationHelper
{
public:
	static constexpr bool voidReturn{ std::is_same_v<R, void> };
	static constexpr std::size_t argumentCount{ sizeof...(A) };
	static constexpr std::size_t jumpSize{ JumpTraits<R>::size };

	void initialize(OperationBuildContext& context)
	{}
	
	void executeVoid(OperationContext& context) const
	{
		execute(context.arguments);
	}
	
	void executeRet(OperationContext& context) const
	{
		static constexpr std::size_t retIndex{ sizeof...(A) };
		assert(context.arguments[retIndex] != nullptr);
		static_cast<Value<R>*>(context.arguments[retIndex])->set(execute(context.arguments));
	}
	
	std::size_t executeJump(OperationContext& context) const
	{
		return JumpTraits<R>::index(execute(context.arguments));
	}
	
	static constexpr OperationDescription getDescription()
	{
		const TypeId* retType{ nullptr };
		if constexpr (!voidReturn)
		{
			retType = &Value<R>::typeId;
			if constexpr (O::id.isConstructor)
				retType->updateName(O::id.operationId);
		}
		return { O::id.operationId, retType, argumentTypes, jumpSize };
	}
	
private:
	R execute(Arguments args) const
	{
		return execute(args, std::index_sequence_for<A...>());
	}
	
	template <std::size_t I>
	using ArgType = std::tuple_element_t<I, std::tuple<A...>>;
	
	template <std::size_t I>
	static decltype(auto) getArg(Arguments args)
	{
		return static_cast<Value<ArgType<I>>*>(args[I])->get();
	}

	template<std::size_t ... I>
	R execute(Arguments args, std::index_sequence<I...>) const
	{
		return static_cast<const O&>(*this)(getArg<I>(args)...);
	}

	static std::array<const TypeId*, argumentCount> argumentTypes;
};

template <typename O, typename R, typename ... A>
std::array<const TypeId*, sizeof...(A)> OperationHelper<O, R, A...>::argumentTypes{ &Value<A>::typeId... };


template <auto I, auto O, typename R, typename ... A>
class LambdaOperationHelper : public OperationHelper<LambdaOperationHelper<I, O, R, A...>, R, A...>
{
public:
    R operator()(A ... args) const
    {
        return O(args...);
    }

    static constexpr OperationId id{ I() };
};



class CPPSCRIPT_API OperationV : public CustomOperation
{
public:
	OperationV(CustomOperation op, OperationBuildContext context)
		: CustomOperation(std::move(op)),
		argumentPlaces(std::move(context.argumentPlaces))
	{}

	const std::vector<ValuePlace>& getArgumentPlaces() const
	{
		return argumentPlaces;
	}

private:
	std::vector<ValuePlace> argumentPlaces;
};


template <typename ... O>
class OperationVBuilder : public OperationBuilder<>
{
public:
	using OperationType = OperationV;
	using Base = OperationBuilder<>;

	OperationVBuilder()
	{
		addCustomOperations<O...>();
	}
	
	OperationType build(OperationBuildContext context) const
	{
		auto op{ std::get<CustomOperation>(Base::builders[context.index](context)) };
		return { std::move(op), std::move(context) };
	}

	std::vector<OperationType> build(std::vector<OperationBuildContext> contexts) const
	{
		std::vector<OperationType> operations;
		operations.reserve(contexts.size());
		for (OperationBuildContext& context : contexts)
			operations.push_back(build(std::move(context)));
		return operations;
	}
};

}