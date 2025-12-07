#include <gmock/gmock.h>

#include <CppScript/OperationResolver.h>
#include <CppScript/CoreOperations.h>
#include "OperationResolverProxy.h"
#include "TestUtils.h"

using namespace CppScript;
using namespace CppScriptTest;

TEST(OperationResolverTest, ResolveCoreOperations)
{
    CoreOperationBuilder builder;
    OperationResolverProxy resolver(builder.getResolver());
    TypeFrames frames;
    getFrame(frames, ValuePlace::Type::Constants) = {&Value<const std::string&>::typeId};
    OperationResolverProxy::ResolutionContext context{ frames };
    EXPECT_TRUE(resolver.resolveOperation({{"int"}, ValuePlace{ValuePlace::Type::Local, 0}, {{ValuePlace::Type::Constants, 0}},
        {1}, {32,14}}, context));
    auto& resolved = context.blockContext.operations[0];
    EXPECT_EQ(resolved.index, 0);
    EXPECT_THAT(resolved.argumentPlaces, testing::ElementsAre(ValuePlace{ValuePlace::Type::Constants, 0}, ValuePlace{ValuePlace::Type::Local, 0}));
    EXPECT_THAT(resolved.jumps, testing::ElementsAre(1));
    EXPECT_EQ(resolved.location, (OperationLocation{32,14}));
    EXPECT_THAT(getFrame(frames, ValuePlace::Type::Local), testing::ElementsAre(&Value<IntValue>::typeId));
}

TEST(OperationResolverTest, AddCustomOperation)
{
    CoreOperationBuilder builder;
    const std::size_t opIndex = builder.getResolver().getDescriptions().size();
    builder.addCustomOperations<LambdaOperationHelper<[](){ return Id{ "testOp", "testMod" }; },
        [](const int a){ return a + 1; }, int, const int>>();
    OperationResolverProxy resolver(builder.getResolver());
    TypeFrames frames;
    getFrame(frames, ValuePlace::Type::Module) = {&Value<int>::typeId};
    OperationResolverProxy::ResolutionContext context{ frames };
    EXPECT_TRUE(resolver.resolveOperation({{"testOp", "testMod"}, ValuePlace{ValuePlace::Type::Local, 1}, {{ValuePlace::Type::Module, 0}},
        {1}, {54,0}}, context));
    auto& resolved = context.blockContext.operations[0];
    EXPECT_EQ(resolved.index, opIndex);
    EXPECT_THAT(resolved.argumentPlaces, testing::ElementsAre(ValuePlace{ValuePlace::Type::Module, 0}, ValuePlace{ValuePlace::Type::Local, 1}));
    EXPECT_THAT(resolved.jumps, testing::ElementsAre(1));
    EXPECT_EQ(resolved.location, (OperationLocation{54,0}));
    EXPECT_THAT(getFrame(frames, ValuePlace::Type::Local), testing::ElementsAre(nullptr, &Value<int>::typeId));
}

TEST(OperationResolverTest, ResolveOperationBlock)
{
    CoreOperationBuilder builder;
    OperationBlockResolutionData blockData{ {}, makeConstants(IntValue{ 42 }, IntValue{ 60 }, IntValue{ 5 }),
        { { {"="}, ValuePlace{ ValuePlace::Type::Local, 0 }, { { ValuePlace::Type::Constants, 0 } }, { 1 }, {0, 1} },
        { {"<=>"}, {}, { { ValuePlace::Type::Local, 0 }, { ValuePlace::Type::Constants, 1 } }, { 1, -2, 2 }, {1, 0} },
        { {"+="}, {}, { { ValuePlace::Type::Local, 0 }, { ValuePlace::Type::Constants, 2 } }, { 4 }, {2, 5} } }};
    TypeFrames frames;
    const auto& resolver = builder.getResolver();
    const auto& descrs = resolver. getDescriptions();
    const auto result = resolver.resolve(std::move(blockData), frames);
    EXPECT_TRUE(std::holds_alternative<OperationBlockBuildContext>(result));
    const auto& buildContext = std::get<OperationBlockBuildContext>(result);
    ASSERT_EQ(buildContext.valueTypes.size(), 1);
    EXPECT_EQ(buildContext.valueTypes[0], &Value<IntValue>::typeId);
    ASSERT_EQ(buildContext.constantValues.size(), 3);
    EXPECT_EQ(static_cast<const Value<const IntValue>&>(*buildContext.constantValues[0]).get(), 42_I);
    EXPECT_EQ(static_cast<const Value<const IntValue>&>(*buildContext.constantValues[1]).get(), 60_I);
    EXPECT_EQ(static_cast<const Value<const IntValue>&>(*buildContext.constantValues[2]).get(), 5_I);
    ASSERT_EQ(buildContext.operations.size(), 3);
    EXPECT_EQ(descrs[buildContext.operations[0].index].id, Id{ "=" });
    EXPECT_THAT(buildContext.operations[0].argumentPlaces,
        testing::ElementsAre(ValuePlace{ValuePlace::Type::Constants, 0}, ValuePlace{ValuePlace::Type::Local, 0}));
    EXPECT_THAT(buildContext.operations[0].jumps, testing::ElementsAre(1));
    EXPECT_EQ(buildContext.operations[0].location, (OperationLocation{0, 1}));
    EXPECT_EQ(descrs[buildContext.operations[1].index].id, Id{ "<=>" });
    EXPECT_THAT(buildContext.operations[1].argumentPlaces,
        testing::ElementsAre(ValuePlace{ValuePlace::Type::Local, 0}, ValuePlace{ValuePlace::Type::Constants, 1}));
    EXPECT_THAT(buildContext.operations[1].jumps, testing::ElementsAre(1, -2, 2));
    EXPECT_EQ(buildContext.operations[1].location, (OperationLocation{1, 0}));
    EXPECT_EQ(descrs[buildContext.operations[2].index].id, Id{ "+=" });
    EXPECT_THAT(buildContext.operations[2].argumentPlaces,
        testing::ElementsAre(ValuePlace{ValuePlace::Type::Local, 0}, ValuePlace{ValuePlace::Type::Constants, 2}));
    EXPECT_THAT(buildContext.operations[2].jumps, testing::ElementsAre(4));
    EXPECT_EQ(buildContext.operations[2].location, (OperationLocation{2, 5}));
 }