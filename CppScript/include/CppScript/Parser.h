#pragma once

#include <CppScript/Definitions.h>
#include <CppScript/Value.h>
#include <CppScript/IntValue.h>
#include <CppScript/OperationResolver.h>
#include <CppScript/ParserUtils.h>
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

struct Indent : Separator {};

struct Identifier : peg::seq<peg::identifier, peg::star<peg::one<'.'>, peg::identifier>> {};

struct AssignedIdentifier : Identifier {};

struct Sign : peg::one<'+', '-'> {};

struct Integer : peg::seq<peg::opt<Sign>, peg::plus<peg::digit>> {};

struct Expression;

struct AssignOperator : peg::sor<peg::string<'='>, peg::string<'+','='>, peg::string<'-','='>,
    peg::string<'*','='>, peg::string<'/','='>, peg::string<'%','='>> {};

struct CompareOperator : peg::sor<peg::string<'=','='>, peg::string<'!','='>,
    peg::string<'<'>, peg::string<'<','='>, peg::string<'>'>, peg::string<'<','='>> {};

struct CompareExpression : peg::seq<Expression, Separator, CompareOperator, Separator, Expression> {};

struct UnaryOperator : peg::one<'-'> {};
struct BinaryOperator : peg::one<'+','-','*','/','%'> {};

struct BracketExpression : peg::seq<peg::one<'('>, Separator, Expression, Separator, peg::one<')'>> {};

struct ExpressionItem : peg::seq<peg::opt<UnaryOperator>, Separator, peg::sor<Identifier, Integer, BracketExpression>> {};

struct Expression : peg::seq<ExpressionItem, peg::star<peg::seq<Separator, BinaryOperator, Separator, ExpressionItem>>> {};

struct Assignment : peg::seq<AssignedIdentifier, Separator, AssignOperator, Separator, Expression> {};

struct Condition : peg::seq<peg::string<'i','f'>, Separator, CompareExpression, Separator, peg::one<':'>> {};

struct Statement : peg::sor<Condition, Assignment> {};

struct CodeLine : peg::seq<Indent, Statement, peg::eolf> {};

struct CodeBlock : peg::plus<CodeLine> {};


struct BuildContext
{
    enum class AssignedPlace { None, New, Existing };
    AssignedPlace assignedPlaceState{ AssignedPlace::None };
    ValuePlace assignedPlace;
    struct Operator
    {
    	std::string id;
        unsigned short priority{ 0 };
        OperationLocation location;
    };
    std::vector<Operator> currentOperators;
    std::vector<Operator> currentCompareOperators;
    std::vector<ValuePlace> currentOperands;
    struct JumpPosition
    {
    	int operationIndex;
    	unsigned short jumpIndex;
    };
    static constexpr std::size_t jumpGroups{ EnumTraits<bool>::size };
    using JumpPositions = std::array<std::vector<JumpPosition>, jumpGroups>;
    JumpPositions currentJumps;
    std::string currentIndent;
    enum class IndentType { Module, Def, If, Else, For, While };
    struct IndentBlock
    {
    	IndentType indentType;
        static constexpr std:: size_t initSize{ std::numeric_limits<std:: size_t>::max() };
        std::size_t indentSize{ initSize };
        JumpPositions jumps;
     };
    std::vector<IndentBlock> indentBlocks{ { IndentType::Module } };
	std::vector<std::unique_ptr<ValueBase>> constantValues;
	using NamePlaceMap = std::unordered_map<std::string, ValuePlace> ;
    NamePlaceMap constantPlaces;
    NamePlaceMap variablePlaces;
    std::vector<NamePlaceMap::iterator> localPlaces;
    OperationBlockResolutionData operationBlock;
    
    ValuePlace addLocalPlace(const std::string_view identifier) 
    {
    	const ValuePlace place{ ValuePlace::Type::Local, localPlaces.size() };
        auto identifierIt = variablePlaces.end();
        if (!identifier.empty())
            identifierIt = variablePlaces.emplace(identifier, place).first;
        localPlaces.push_back(identifierIt);
        return place;
    }
    
    void replacePlace(const ValuePlace source, const ValuePlace target) 
    {
        assert(source.placeType == ValuePlace::Type::Local && target.placeType == ValuePlace::Type::Local
            && source.index == localPlaces.size() - 1);
        assert(localPlaces[source.index] == variablePlaces.end());
        assert(operationBlock.operations.back().returnPlace.has_value()
            && operationBlock.operations.back().returnPlace->placeType == ValuePlace::Type::Local
            && operationBlock.operations.back().returnPlace->index == source.index);
        operationBlock.operations.back().returnPlace = target;
        localPlaces.pop_back();
    }
    
    std::vector<ValuePlace> popOperands(const std::size_t count)
    {
    	const auto startPlace{ std::next(currentOperands.begin(), count > currentOperands.size() ? 0 : currentOperands.size() - count) };
    	std::vector<ValuePlace> result{ startPlace, currentOperands.end() };
        currentOperands.erase(startPlace, currentOperands.end());
        return result;
    }

    void setJumpsToNextOperation(JumpPositions& jumps, const std::size_t group)
    {
        const int nextOperationIndex = static_cast<int>(operationBlock.operations.size());
    	for (const auto jumpPos : jumps[group])
            operationBlock.operations[jumpPos.operationIndex].jumps[jumpPos.jumpIndex]
                = nextOperationIndex - jumpPos.operationIndex;
        jumps[group].clear();
    }
    
    void flushOperators( const unsigned short flushPriority = 1)
    {
        auto operIt = currentOperators.rbegin();
        for (; operIt != currentOperators.rend() && operIt->priority >= flushPriority; ++operIt)
        {
			auto operands{ popOperands(2) };
			assert(operands.size() == 2);
			ValuePlace resultPlace = addLocalPlace("");
			currentOperands.push_back(resultPlace);
            operationBlock.operations.push_back({ { operIt->id }, resultPlace, std::move(operands), { 1 }, operIt->location });
        }
        currentOperators.erase(operIt.base(), currentOperators.end());
	}
};

OperationLocation makeLocation(const peg::position& pos)
{
    return OperationLocation{ static_cast<unsigned short>(pos.line), static_cast<unsigned short>(pos.column) };
}

template< typename Rule >
struct BuildAction : peg::nothing<Rule>
{};

template<>
struct BuildAction<Indent>
{
    template <typename ParseInput>
    static void apply(const ParseInput& in, BuildContext& context)
    {
    	const auto& indent = in.string();
    	if (context.indentBlocks.back().indentSize == BuildContext::IndentBlock::initSize)
        {
            context.indentBlocks.back().indentSize = indent.size();
        }
        else if (indent.size() < context.currentIndent.size())
        {
            auto indentIt = context.indentBlocks.rbegin();
        	for (; indentIt->indentSize > indent.size(); ++indentIt)
                context.setJumpsToNextOperation(indentIt->jumps, EnumTraits<bool>::index(false));
            context.indentBlocks.erase(indentIt.base(), context.indentBlocks.end());
        }
        context.currentIndent = indent;
        std::cout << "Indent " << indent.size() << std::endl;
    }
};

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
struct BuildAction<Identifier>
{
    template <typename ParseInput>
    static void apply(const ParseInput& in, BuildContext& context)
    {
    	std::string identifier = in.string();
    	auto foundId = context.variablePlaces.find(identifier);
        context.currentOperands.push_back(foundId->second);
        std::cout << "identifier " << in.string() << std::endl;
    }
};

template<>
struct BuildAction<AssignedIdentifier>
{
    template <typename ParseInput>
    static void apply(const ParseInput& in, BuildContext& context)
    {
    	std::string identifier = in.string();
    	auto foundId = context.variablePlaces.find(identifier);
    	if (foundId == context.variablePlaces.end())
    	{
            context.assignedPlaceState = BuildContext::AssignedPlace::New;
			const ValuePlace place{ context.addLocalPlace(std::move(identifier)) };
            foundId = context.localPlaces[place.index];
        }
        else
        {
            context.assignedPlaceState = BuildContext::AssignedPlace::Existing;
        }
        context.assignedPlace = foundId->second;
        std::cout << "assigned identifier " << in.string() << std::endl;
    }
};

template<>
struct BuildAction<UnaryOperator>
{
    template <typename ParseInput>
    static void apply(const ParseInput& in, BuildContext& context)
    {
        std::cout << "UnaryOperator " << in.string() << std::endl;
    }
};


struct BinaryOperatorData
{
    constexpr BinaryOperatorData() = default;

	constexpr BinaryOperatorData(const unsigned short priority, const bool leftAssociative = true)
	    : priority(priority), flushPriority(priority + (leftAssociative ? 0 : 1))
	{}
	
    unsigned short priority{ 0 };
    unsigned short flushPriority{ 0 };
};

using BinaryOperatorsMap = TokenMap<BinaryOperatorData, 8, 
    [](std::string_view token) -> unsigned short
    {
        return (token[0] >> 1 ^ token[0] << 2) & 7;
    }>;

constexpr BinaryOperatorsMap binaryOperators
    {
        { "+", { 2 } },
        { "-", { 2 } },
        { "*", { 3 } },
        { "/", { 3 } },
        { "%", { 3 } }
    };

template<>
struct BuildAction<BinaryOperator>
{
    template <typename ParseInput>
    static void apply(const ParseInput& in, BuildContext& context)
    {
        const BinaryOperatorData& operData = binaryOperators[in.string()];
        context.flushOperators(operData.flushPriority);
        context.currentOperators.push_back({ { in.string() }, operData.priority, makeLocation(in.position()) });
        std::cout << "BinaryOperator " << in.string() << std::endl;
    }
};

template<>
struct BuildAction<BracketExpression>
{
    template <typename ParseInput>
    static void apply(const ParseInput& in, BuildContext& context)
    {
        std::cout << "BracketExpression " << in.string() << std::endl;
    }
};

template<>
struct BuildAction<ExpressionItem>
{
    template <typename ParseInput>
    static void apply(const ParseInput& in, BuildContext& context)
    {
        std::cout << "ExpressionItem " << in.string() << std::endl;
    }
};

template<>
struct BuildAction<Expression>
{
    template <typename ParseInput>
    static void apply(const ParseInput& in, BuildContext& context)
    {
        context.flushOperators();
        std::cout << "Expression " << in.string() << std::endl;
    }
};

template<>
struct BuildAction<AssignOperator>
{
    template <typename ParseInput>
    static void apply(const ParseInput& in, BuildContext& context)
    {
        context.currentOperators.push_back({ { in.string() } });
        std::cout << "AssignOperator " << in.string() << std::endl;
    }
};

template<>
struct BuildAction<Assignment>
{
    template <typename ParseInput>
    static void apply(const ParseInput& in, BuildContext& context)
    {
    	const auto& currOp{ context.currentOperators.back() };
        const bool simpleAssign = currOp.id == "=";
        if (simpleAssign)
        {
        	auto sourcePlace{ context.popOperands(1) };
            if (sourcePlace[0]. placeType == ValuePlace::Type::Local && context.localPlaces[sourcePlace[0].index] == context.variablePlaces.end())
                context.replacePlace(sourcePlace[0], context.assignedPlace);
            else
                context.operationBlock.operations.push_back({ { "=" }, context.assignedPlace,
                    std::move(sourcePlace), { 1 }, makeLocation(in.position()) });
        }
        else
        {
            auto args = context.popOperands(1);
            args.insert(args.begin(), context.assignedPlace);
            context.operationBlock.operations.push_back({ { currOp.id }, {},
                std::move(args), { 1 }, makeLocation(in.position()) });
        }
        context.assignedPlaceState = BuildContext::AssignedPlace::None;
        context.currentOperators.pop_back();
        std::cout << "Assignment " << in.string() << std::endl;
    }
};

class CompareOperatorJumps
{
public:
    constexpr CompareOperatorJumps() = default;

    constexpr CompareOperatorJumps(std::initializer_list<bool> logicResults)
    {
        std::transform(logicResults.begin(), logicResults.end(), jumpIndices.begin(),
            [](bool logicResult) { return logicResult ? 1 : 0; });
    }

    void addJumps(BuildContext::JumpPositions& target) const
    {
    	for (unsigned short index = 0; index < jumpIndices.size(); ++index)
            target[jumpIndices[index]].push_back({ 0, index });
    }

private:
    using JumpArray = std::array<unsigned short, EnumTraits<std::strong_ordering>::size>;

    JumpArray jumpIndices{};
};

using CompareOperatorJumpsMap = TokenMap<CompareOperatorJumps, 8, 
    [](std::string_view token) -> unsigned short
    {
        return (token.length() == 2 ? (token[0] ^ token[1] << 2) : token[0]) & 7;
    }>;

constexpr CompareOperatorJumpsMap compareOperatorJumps
    {
        { "==", { false, true, false } },
        { "!=", { true, false, true } },
        { "<", { true, false, false } },
        { "<=", { true, true, false } },
        { ">", { false, false, true } },
        { ">=", { false, true, true } }
    };

template<>
struct BuildAction<CompareOperator>
{
    template <typename ParseInput>
    static void apply(const ParseInput& in, BuildContext& context)
    {
        context.currentCompareOperators.push_back({ { in.string() }, 1, makeLocation(in.position()) });
        //context.currentJumps[0].push_back({ 0, 0 });
        //context.currentJumps[1].push_back({ 0, 1 });
        //compareOperatorJumps[in.string()].addJumps(context.currentJumps);
        std::cout << "CompareOperator " << in.string() << std::endl;
    }
};

template<>
struct BuildAction<CompareExpression>
{
    template <typename ParseInput>
    static void apply(const ParseInput& in, BuildContext& context)
    {
        const int opIndex = static_cast<int>(context.operationBlock.operations.size());
        const int nextOpIndex = opIndex + 1;
        const auto& compareOp = context.currentCompareOperators.back();
        context.operationBlock.operations.push_back({ { compareOp.id }, {},
            context.popOperands(2), { 1, 1 }, compareOp.location });
        unsigned short index{ 0 };
        for (auto& jumpGroup : context.currentJumps)
            jumpGroup.push_back({ opIndex, index++ });
        context.currentCompareOperators.pop_back();
        std::cout << "CompareExpression " << in.string() << std::endl;
    }
};

template<>
struct BuildAction<Condition>
{
    template <typename ParseInput>
    static void apply(const ParseInput& in, BuildContext& context)
    {
        context.setJumpsToNextOperation(context.currentJumps, EnumTraits<bool>::index(true));
        context.indentBlocks.push_back({ BuildContext::IndentType::If });
        constexpr auto falseJumpIndex = EnumTraits<bool>::index(false);
        context.indentBlocks.back().jumps[falseJumpIndex] = std::move(context.currentJumps[falseJumpIndex]);
        std::cout << "Condition " << in.string() << std::endl;
    }
};

template<>
struct BuildAction<CodeLine>
{
    template <typename ParseInput>
    static void apply(const ParseInput& in, BuildContext& context)
    {
        std::cout << "CodeLine " << in.string() << std::endl;
    }
};

template<>
struct BuildAction<CodeBlock>
{
    template <typename ParseInput>
    static void apply(const ParseInput& in, BuildContext& context)
    {
        std::cout << "CodeBlock " << in.string() << std::endl;
    }
};

}