#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <CppScript/Execution.h>
#include <CppScript/FunctionDef.h>

class FunctionMock : public CppScript::Function
{
public:
    MOCK_METHOD(void, execute, (CppScript::ExecutionContext& context), (const, override));
};

using namespace CppScript;

TEST(ExecutionTest, RunCode)
{
    ExecutionContext context;

    auto fnc1 = std::make_unique<FunctionMock>();
    auto fnc2 = std::make_unique<FunctionMock>();
    auto fnc3 = std::make_unique<FunctionMock>();
    
    testing::InSequence seq;
    EXPECT_CALL(*fnc1, execute(testing::Ref(context))).Times(1);
    EXPECT_CALL(*fnc2, execute(testing::Ref(context))).Times(1);
    EXPECT_CALL(*fnc3, execute(testing::Ref(context))).Times(1);
    CodeBlock testCode;
    testCode.operations.push_back(std::move(fnc1));
    testCode.operations.push_back(std::move(fnc2));
    testCode.operations.push_back(std::move(fnc3));

    context.run(testCode);
}

TEST(ExecutionTest, RunCodeWithJumps)
{
    ExecutionContext context;

    auto fnc1 = std::make_unique<FunctionMock>();
    auto fnc2 = std::make_unique<FunctionMock>();
    auto fnc3 = std::make_unique<FunctionMock>();

    testing::InSequence seq;
    EXPECT_CALL(*fnc1, execute(testing::Ref(context))).Times(1);
    EXPECT_CALL(*fnc2, execute(testing::Ref(context)))
        .WillOnce([](ExecutionContext& cont){ cont.jump(-1); });
    EXPECT_CALL(*fnc1, execute(testing::Ref(context)))
        .WillOnce([](ExecutionContext& cont){ cont.jump(2); });
    EXPECT_CALL(*fnc3, execute(testing::Ref(context))).Times(1);
    CodeBlock testCode;
    testCode.operations.push_back(std::move(fnc1));
    testCode.operations.push_back(std::move(fnc2));
    testCode.operations.push_back(std::move(fnc3));

    context.run(testCode);
}
