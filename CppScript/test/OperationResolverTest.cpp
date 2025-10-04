#include <gmock/gmock.h>

#include <CppScript/OperationResolver.h>
#include <CppScript/CoreOperations.h>
#include "TestUtils.h"

using namespace CppScript;
using namespace CppScriptTest;

TEST(OperationResolverTest, ResolveCoreOperations)
{
    CoreOperationBuilder builder;
    OperationResolver& resolver = builder.getResolver();
    OperationResolver::Result result = resolver.resolve({ "int" }, { &Value<std::string>::typeId });
    EXPECT_TRUE(std::holds_alternative<OperationResolver::Resolved>(result));
    const auto& resolved = std::get<OperationResolver::Resolved>(result);
    EXPECT_EQ(resolved.index, 0);
    EXPECT_EQ(resolved.description.id, Id{ "int" });
    EXPECT_EQ(*resolved.description.returnType, Value<IntValue>::typeId);
    EXPECT_THAT(span2Vector(resolved.description.argumentTypes), testing::ElementsAre(&Value<const std::string&>::typeId));
    EXPECT_EQ(resolved.description.jumpCount, 1);
}

TEST(OperationResolverTest, AddCustomOperation)
{
    CoreOperationBuilder builder;
    OperationResolver& resolver = builder.getResolver();
    const std::size_t opIndex = resolver.getCount();
    builder.addCustomOperation<CustomOperationWrapper<LambdaOperationHelper<[](){ return Id{ "testOp", "testMod" }; },
        [](const int a){ return a + 1; }, int, const int>>>();
    OperationResolver::Result result = resolver.resolve({ "testOp", "testMod" }, { &Value<int>::typeId });
    EXPECT_TRUE(std::holds_alternative<OperationResolver::Resolved>(result));
    const auto& resolved = std::get<OperationResolver::Resolved>(result);
    EXPECT_EQ(resolved.index, opIndex);
    EXPECT_EQ(resolved.description.id, (Id{ "testOp", "testMod" }));
    EXPECT_EQ(*resolved.description.returnType, Value<int>::typeId);
    EXPECT_THAT(span2Vector(resolved.description.argumentTypes), testing::ElementsAre(&Value<const int>::typeId));
    EXPECT_EQ(resolved.description.jumpCount, 1);
}