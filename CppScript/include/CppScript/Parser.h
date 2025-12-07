#pragma once

#include <CppScript/Definitions.h>
#include <CppScript/Value.h>
#include <CppScript/IntValue.h>
#include <CppScript/OperationResolver.h>
#include <tao/pegtl.hpp>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

#include <iostream>

namespace peg = tao::pegtl;

namespace CppScript::Grammar
{

struct Separator : peg::star<peg::space> {};

struct Identifier : peg::identifier {};

struct Sign : peg::one<'+', '-'> {};

struct Integer : peg::seq<peg::opt<Sign>, peg::plus<peg::digit>> {};

struct Expression : peg::sor<Identifier, Integer> {};

struct Assignment : peg::seq<Identifier, Separator, peg::one<'='>, Separator, Expression> {};

struct BuildContext
{
    Id currentOperation;
    std::vector<ValuePlace> currentOperands;
	std::vector<std::unique_ptr<ValueBase>> constantValues;
    std::unordered_map<std::string, ValuePlace> constantPlaces;
    std::vector<TypeId*> variableTypes;
    std::unordered_map<std::string, ValuePlace> variablePlaces;
    OperationBlockResolutionData operationBlock;
};

template< typename Rule >
struct BuildAction : peg::nothing<Rule>
{};

template<>
struct BuildAction<Integer>
{
    template <typename ParseInput>
    static void apply(const ParseInput& in, BuildContext& context)
    {
        std::string literal = in.string();
		auto foundConst = context.constantPlaces.find(literal);
        if (foundConst == context.constantPlaces.end())
        {
            auto value = std::make_unique<Value<const IntValue>>();
			value->set(IntValue{literal});
			const ValuePlace place{ ValuePlace::Type::Constants, context.constantValues.size() };
			context.constantValues.push_back(std::move(value));
            foundConst = context.constantPlaces.emplace(std::move(literal), place).first;
        }
		context.currentOperands.push_back(foundConst->second);
        std::cout << "Integer " << in.string() << std::endl;
    }
};

template<>
struct BuildAction<Expression>
{
    template <typename ParseInput>
    static void apply(const ParseInput& in, BuildContext& context)
    {
        std::cout << "Expression " << in.string() << std::endl;
    }
};

template<>
struct BuildAction<Identifier>
{
    template <typename ParseInput>
    static void apply(const ParseInput& in, BuildContext& context)
    {
    	std::string identifier = in.string();
    	auto foundId = context.variablePlaces.find(identifier);
    	if (foundId == context.variablePlaces.end())
    	{
			const ValuePlace place{ ValuePlace::Type::Local, context.variableTypes.size() };
			context.variableTypes.push_back(nullptr);
            foundId = context.variablePlaces.emplace(std::move(identifier), place).first;
        }
        context.currentOperands.push_back(foundId->second);
        std::cout << "identifier " << in.string() << std::endl;
    }
};

template<>
struct BuildAction<Assignment>
{
    template <typename ParseInput>
    static void apply(const ParseInput& in, BuildContext& context)
    {
        OperationResolutionData opRes;
        opRes.operationId = Id{ "=" };
        opRes.argumentPlaces = { context.currentOperands[1], context.currentOperands[0] };
        context.operationBlock.operations.push_back(std::move(opRes));
        context.currentOperands.clear();
        std::cout << "Assignment " << in.string() << std::endl;
    }
};

}