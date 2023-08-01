#include <gtest/gtest.h>

#include <CppScript/FunctionDef.h>

using namespace CppScript;


class TestedClass
{
public:
    int a;

    void set(const TestedClass& other)
    {
        a = other.a;
    }

    void add(const TestedClass& other)
    {
        a += other.a;
    }

    int& get()
    {
        return a;
    }

    bool equal(const TestedClass& other) const
    {
        return a == other.a;
    }
};

float diff(float x, float y)
{
    return x - y;
}

class FunctionDefFixture : public testing::Test
{
protected:
    void SetUp() override
    {
        testModule.defFunction("diff", &diff)
            .defFunction("set", &TestedClass::set)
            .defFunction("add", &TestedClass::add)
            .defFunction("get", &TestedClass::get)
            .defFunction("==", &TestedClass::equal);
    }

    Module testModule{"testModule"};
};

TEST_F(FunctionDefFixture, FunctionCreationAndExecution)
{
    CodeBlock code;
    const auto place1 = code.addPlaceValue(10.0F);
    const auto place2 = code.addPlaceValue(2.5F);
    const auto place3 = code.addPlaceType(SpecTypeValueHolder<float>::specTypeId);
    ExecutionContext context;

    FunctionContext funcCont{"diff", {}, place3, {place1, place2}, {}, &code};
    code.operations.push_back(testModule.buildFunction(funcCont));
    context.run(code);
    EXPECT_EQ(static_cast<TypeValueHolder<float>&>(context.get(place3)).get(), 7.5F);
}

TEST_F(FunctionDefFixture, MethodDefConstructionAndCall)
{
    CodeBlock code;
    const auto place1 = code.addPlaceValue(TestedClass{10});
    const auto place2 = code.addPlaceValue(TestedClass{5});
    const auto place3 = code.addPlaceValue(TestedClass{2});
    const auto place4 = code.addPlaceValue(int(4));

    ExecutionContext context;
    FunctionContext funcCont{"add", {}, {PlaceType::Void}, {place1, place3}, {}, &code};
    code.operations.push_back(testModule.buildFunction(funcCont));
    FunctionContext funcCont2{"get", {}, place4, {place3}, {}, &code};
    code.operations.push_back(testModule.buildFunction(funcCont2));

    context.run(code);
    
    EXPECT_EQ(static_cast<TypeValueHolder<TestedClass>&>(context.get(place1)).get().get(), 12);
    EXPECT_EQ(static_cast<TypeValueHolder<int>&>(context.get(place4)).get(), 2);
}

TEST_F(FunctionDefFixture, MethodWithJump)
{
    CodeBlock code;
    const auto place1 = code.addPlaceValue(TestedClass{8});
    const auto place2 = code.addPlaceValue(TestedClass{5});
    const auto place3 = code.addPlaceValue(TestedClass{});
    ExecutionContext context;

    FunctionContext funcCont{"==", {FunctionOptions::Jump}, {PlaceType::Void}, {place1, place2}, {1, 2}, &code};
    code.operations.push_back(testModule.buildFunction(funcCont));
    funcCont.name = "add";
    funcCont.options = {};
    funcCont.argPlaces = {place1, place2};
    code.operations.push_back(testModule.buildFunction(funcCont));
    funcCont.name = "set";
    funcCont.argPlaces = {place3, place1};
    code.operations.push_back(testModule.buildFunction(funcCont));

    context.run(code);

    EXPECT_EQ(static_cast<TypeValueHolder<TestedClass>&>(context.get(place3)).get().get(), 8);
}

TEST_F(FunctionDefFixture, MethodWithJump2)
{
    CodeBlock code;
    const auto place1 = code.addPlaceValue(TestedClass{5});
    const auto place2 = code.addPlaceValue(TestedClass{5});
    const auto place3 = code.addPlaceValue(TestedClass{});
    ExecutionContext context;

    FunctionContext funcCont{"==", {FunctionOptions::Jump}, {PlaceType::Void}, {place1, place2}, {1, 2}, &code};
    code.operations.push_back(testModule.buildFunction(funcCont));
    funcCont.name = "add";
    funcCont.options = {};
    funcCont.argPlaces = {place1, place2};
    code.operations.push_back(testModule.buildFunction(funcCont));
    funcCont.name = "set";
    funcCont.argPlaces = {place3, place1};
    code.operations.push_back(testModule.buildFunction(funcCont));

    context.run(code);

    EXPECT_EQ(static_cast<TypeValueHolder<TestedClass>&>(context.get(place3)).get().get(), 10);
}