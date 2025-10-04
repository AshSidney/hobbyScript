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


template <typename ... O>
class Operation
{
public:
	using OperationVariant = std::variant<CustomOperation, O...>;

	Operation(OperationVariant op, OperationBuildContext context)
		: operation(std::move(op)),
		argumentPlaces(std::move(context.argumentPlaces))
	{}

	void execute(OperationContext& context) const
	{
		std::visit([&context](const auto& op){ op.execute(context); }, operation);
	}
	
	const std::vector<ValuePlace>& getArgumentPlaces() const
	{
		return argumentPlaces;
	}

private:
	OperationVariant operation;
	
	std::vector<ValuePlace> argumentPlaces;
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


template <typename ... O>
class OperationBuilder
{
public:
	using OperationType = Operation<O...>;

	OperationType build(OperationBuildContext context) const
	{
		assert(context.index < builders.size());
		assert(resolver.validate(context));
		auto op{ builders[context.index](context) };
		return{ std::move(op), std::move(context) };
	}

	OperationResolver& getResolver()
	{
		return resolver;
	}

	template <typename CO>
	void addCustomOperation()
	{
		resolver.addDescription(CO::getDescription());

		builders.push_back([](OperationBuildContext& context)
		{
			auto customOp = std::make_unique<CO>();
			customOp->initialize(context);
			return OperationVariant{ CustomOperation{ std::move(customOp) } };
		});
	}
	
	template <typename ... CO>
	void addCustomOperations()
	{
		(addCustomOperation<CO>(), ...);
	}

protected:
	using OperationVariant = OperationType::OperationVariant;
	using Builder = OperationVariant(OperationBuildContext&);

	static constexpr std::vector<Builder*> createBuilders()
	{
		std::vector<Builder*> opBuilders;
		( opBuilders.push_back([](OperationBuildContext& context)
			{
				O op;
				op.initialize(context);
				return OperationVariant{ std::move(op) };
			}), ...);
		return opBuilders;
	}

	std::vector<Builder*> builders{ createBuilders() };
	OperationResolver resolver{ std::vector<OperationDescription>{ O::getDescription() ... } };
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
	void initialize(OperationBuildContext& context)
	{
		evaluateJump = context.jumps.size() > 1;
		std::copy(context.jumps.begin(), context.jumps.end(), jumpTable.begin());
	}

	void execute(OperationContext& context) const
	{
		context.nextStep = jumpTable[0];
		if constexpr (voidReturn)
		{
			executeRet(context.arguments);
		}
		else if constexpr (jumpSize > 1)
		{
			if (evaluateJump)
				context.nextStep = jumpTable[JumpTraits<R>::index(executeRet(context.arguments))];
			else
				execute(context.arguments);
		}
		else
		{
			execute(context.arguments);
		}
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
	void execute(Arguments args) const
	{
		static constexpr std::size_t retIndex{ sizeof...(A) };
		if (args[retIndex] == nullptr)
		{
			executeRet(args);
		}
		else
		{
			static_cast<Value<R>*>(args[retIndex])->set(executeRet(args));
		}
	}

	R executeRet(Arguments args) const
	{
		return executeRet(args, std::index_sequence_for<A...>());
	}
	
	template <std::size_t I>
	using ArgType = std::tuple_element_t<I, std::tuple<A...>>;
	
	template <std::size_t I>
	static decltype(auto) getArg(Arguments args)
	{
		return static_cast<Value<ArgType<I>>*>(args[I])->get();
	}

	template<std::size_t ... I>
	R executeRet(Arguments args, std::index_sequence<I...>) const
	{
		return static_cast<const O&>(*this)(getArg<I>(args)...);
	}

	static constexpr std::size_t jumpSize{ JumpTraits<R>::size };
	
	static constexpr bool voidReturn{ std::is_same_v<R, void> };
	
	bool evaluateJump{ false };
	std::array<int, jumpSize> jumpTable { 1 };

	static std::array<const TypeId*, sizeof...(A)> argumentTypes;
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


template <typename O>
class CustomOperationWrapper : public CustomOperation::Base, public O
{
public:
	void execute(OperationContext& context) const override
	{
		O::execute(context);
	}
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
		addCustomOperations<CustomOperationWrapper<O>...>();
	}
	
	OperationType build(OperationBuildContext context) const
	{
		auto op{ std::get<CustomOperation>(Base::builders[context.index](context)) };
		return { std::move(op), std::move(context) };
	}
};

}