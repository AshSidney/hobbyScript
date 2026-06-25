#include <CppScript/Parser.h>
#include <gmock/gmock.h>
#include "TestUtils.h"
#include "ParserUtilsTest.h"

using namespace CppScript;
using namespace CppScript::Grammar;
using namespace CppScriptTest;

template <typename R>
BuildContext parse(std::string_view source)
{
   BuildContext context;
   peg::string_input in( source, "" );
   peg::parse<R, BuildAction>( in, context );
   return context;
}


TEST(ParserTest, TokenMapsHashFunctions)
{
   TestTokenMap testCompareOperatorJumps(compareOperatorJumps);
   EXPECT_TRUE(testCompareOperatorJumps.validateHashFunction());
   TestTokenMap testBinaryOperators(binaryOperators);
   EXPECT_TRUE(testBinaryOperators.validateHashFunction());
}

TEST(ParserTest, Assignment)
{
   auto result = parse<CodeBlock>("ab=123\ncd = ab");
   ASSERT_EQ(result.operationBlock.operations.size(), 2);
   const auto& op0 = result.operationBlock.operations[0];
   EXPECT_EQ(op0.operationId, Id{"="});
   EXPECT_EQ(*op0.returnPlace, (ValuePlace{ ValuePlace::Type::Local, 0 }));
   EXPECT_THAT(op0.argumentPlaces, testing::ElementsAre(ValuePlace{ ValuePlace::Type::Constants, 0 }));
   EXPECT_THAT(op0.jumps, testing::ElementsAre(1));
   EXPECT_EQ(op0.location, (OperationLocation{1, 1}));
   const auto& op1 = result.operationBlock.operations[1];
   EXPECT_EQ(op1.operationId, Id{"="});
   EXPECT_EQ(*op1.returnPlace, (ValuePlace{ ValuePlace::Type::Local, 1 }));
   EXPECT_THAT(op1.argumentPlaces, testing::ElementsAre(ValuePlace{ ValuePlace::Type:: Local, 0 }));
   EXPECT_THAT(op1.jumps, testing::ElementsAre(1));
   EXPECT_EQ(op1.location, (OperationLocation{2, 1}));
   ASSERT_EQ(result.constantValues.size(), 1);
   EXPECT_EQ(static_cast<Value<const IntValue>&>(*result.constantValues[0]).get(), IntValue{123});
   EXPECT_EQ(result.variablePlaces.size(), 2);
}

TEST(ParserTest, ConditionIndentation)
{
   auto result = parse<CodeBlock>("val=1\nif val > 0:\n    val=5\nval += 1");
   ASSERT_EQ(result.operationBlock.operations.size(), 4);
   const auto& op0 = result.operationBlock.operations[0];
   EXPECT_EQ(op0.operationId, Id{"="});
   EXPECT_EQ(*op0.returnPlace, (ValuePlace{ ValuePlace::Type::Local, 0 }));
   EXPECT_THAT(op0.argumentPlaces, testing::ElementsAre(ValuePlace{ ValuePlace::Type::Constants, 0 }));
   EXPECT_THAT(op0.jumps, testing::ElementsAre(1));
   EXPECT_EQ(op0.location, (OperationLocation{1, 1}));
   const auto& op1 = result.operationBlock.operations[1];
   EXPECT_EQ(op1.operationId, Id{">"});
   EXPECT_FALSE(op1.returnPlace.has_value());
   EXPECT_THAT(op1.argumentPlaces, testing::ElementsAre(ValuePlace{ ValuePlace::Type:: Local, 0 }, ValuePlace{ ValuePlace::Type::Constants, 1 }));
   EXPECT_THAT(op1.jumps, testing::ElementsAre(2, 1));
   EXPECT_EQ(op1.location, (OperationLocation{2, 8}));
   const auto& op2 = result.operationBlock.operations[2];
   EXPECT_EQ(op2.operationId, Id{"="});
   EXPECT_EQ(*op2.returnPlace, (ValuePlace{ ValuePlace::Type::Local, 0 }));
   EXPECT_THAT(op2.argumentPlaces, testing::ElementsAre(ValuePlace{ ValuePlace::Type::Constants, 2 }));
   EXPECT_THAT(op2.jumps, testing::ElementsAre(1));
   EXPECT_EQ(op2.location, (OperationLocation{3, 5}));
   const auto& op3 = result.operationBlock.operations[3];
   EXPECT_EQ(op3.operationId, Id{"+="});
   EXPECT_FALSE(op3.returnPlace.has_value());
   EXPECT_THAT(op3.argumentPlaces, testing::ElementsAre(ValuePlace{ ValuePlace::Type:: Local, 0 }, ValuePlace{ ValuePlace::Type::Constants, 0 }));
   EXPECT_THAT(op3.jumps, testing::ElementsAre(1));
   EXPECT_EQ(op3.location, (OperationLocation{4, 1}));
   ASSERT_EQ(result.constantValues.size(), 3);
   EXPECT_EQ(static_cast<Value<const IntValue>&>(*result.constantValues[0]).get(), IntValue{1});
   EXPECT_EQ(static_cast<Value<const IntValue>&>(*result.constantValues[1]).get(), IntValue{0});
   EXPECT_EQ(static_cast<Value<const IntValue>&>(*result.constantValues[2]).get(), IntValue{5});
   EXPECT_EQ(result.variablePlaces.size(), 1);
}

TEST(ParserTest, Expression)
{
   auto result = parse<CodeBlock>("coef=4\nval = 5 * coef - coef / 2 + 3");
   ASSERT_EQ(result.operationBlock.operations.size(), 5);
   const auto& op0 = result.operationBlock.operations[0];
   EXPECT_EQ(op0.operationId, Id{"="});
   EXPECT_EQ(*op0.returnPlace, (ValuePlace{ ValuePlace::Type::Local, 0 }));
   EXPECT_THAT(op0.argumentPlaces, testing::ElementsAre(ValuePlace{ ValuePlace::Type::Constants, 0 }));
   EXPECT_THAT(op0.jumps, testing::ElementsAre(1));
   EXPECT_EQ(op0.location, (OperationLocation{1, 1}));
   const auto& op1 = result.operationBlock.operations[1];
   EXPECT_EQ(op1.operationId, Id{"*"});
   EXPECT_EQ(*op1.returnPlace, (ValuePlace{ ValuePlace::Type::Local, 2 }));
   EXPECT_THAT(op1.argumentPlaces, testing::ElementsAre(ValuePlace{ ValuePlace::Type::Constants, 1 }, ValuePlace{ ValuePlace::Type:: Local, 0 }));
   EXPECT_THAT(op1.jumps, testing::ElementsAre(1));
   EXPECT_EQ(op1.location, (OperationLocation{2, 9}));
   const auto& op2 = result.operationBlock.operations[2];
   EXPECT_EQ(op2.operationId, Id{"/"});
   EXPECT_EQ(*op2.returnPlace, (ValuePlace{ ValuePlace::Type::Local, 3 }));
   EXPECT_THAT(op2.argumentPlaces, testing::ElementsAre(ValuePlace{ ValuePlace::Type:: Local, 0 }, ValuePlace{ ValuePlace::Type::Constants, 2 }));
   EXPECT_THAT(op2.jumps, testing::ElementsAre(1));
   EXPECT_EQ(op2.location, (OperationLocation{2, 23}));
   const auto& op3 = result.operationBlock.operations[3];
   EXPECT_EQ(op3.operationId, Id{"-"});
   EXPECT_EQ(*op3.returnPlace, (ValuePlace{ ValuePlace::Type::Local, 4 }));
   EXPECT_THAT(op3.argumentPlaces, testing::ElementsAre(ValuePlace{ ValuePlace::Type:: Local, 2 }, ValuePlace{ ValuePlace::Type::Local, 3 }));
   EXPECT_THAT(op3.jumps, testing::ElementsAre(1));
   EXPECT_EQ(op3.location, (OperationLocation{2, 16}));
   const auto& op4 = result.operationBlock.operations[4];
   EXPECT_EQ(op4.operationId, Id{"+"});
   EXPECT_EQ(*op4.returnPlace, (ValuePlace{ ValuePlace::Type::Local, 1 }));
   EXPECT_THAT(op4.argumentPlaces, testing::ElementsAre(ValuePlace{ ValuePlace::Type:: Local, 4 }, ValuePlace{ ValuePlace::Type::Constants, 3 }));
   EXPECT_THAT(op4.jumps, testing::ElementsAre(1));
   EXPECT_EQ(op4.location, (OperationLocation{2, 27}));
   ASSERT_EQ(result.constantValues.size(), 4);
   EXPECT_EQ(static_cast<Value<const IntValue>&>(*result.constantValues[0]).get(), IntValue{4});
   EXPECT_EQ(static_cast<Value<const IntValue>&>(*result.constantValues[1]).get(), IntValue{5});
   EXPECT_EQ(static_cast<Value<const IntValue>&>(*result.constantValues[2]).get(), IntValue{2});
   EXPECT_EQ(static_cast<Value<const IntValue>&>(*result.constantValues[3]).get(), IntValue{3});
   EXPECT_EQ(result.variablePlaces.size(), 2);
   EXPECT_EQ(result.localPlaces.size(), 5);
}
