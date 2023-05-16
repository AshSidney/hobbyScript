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
    SpecTypeValueHolder<float> x, y, r;
    x.set(10.0F);
    y.set(2.5F);
    r.set(0.0F);
    ExecutionContext context;
    context.set({PlaceType::Local, 1}, x);
    context.set({PlaceType::Local, 2}, y);
    context.set({PlaceType::Local, 3}, r);

    FunctionContext funcCont{"diff", FunctionOptions::Default, {PlaceType::Local, 3}, {{PlaceType::Local, 1}, {PlaceType::Local, 2}}};
    auto fnc = testModule.buildFunction(funcCont);
    fnc->execute(context);
    EXPECT_EQ(r.get(), 7.5F);
}

TEST_F(FunctionDefFixture, MethodDefConstructionAndCall)
{
    SpecTypeValueHolder<TestedClass> x, y, z;
    x.set({10});
    y.set({5});
    z.set({2});
    SpecTypeValueHolder<int> w;

    ExecutionContext context;
    context.set({PlaceType::Local, 1}, x);
    context.set({PlaceType::Local, 2}, z);
    FunctionContext funcCont{"add", FunctionOptions::Default, {PlaceType::Void}, {{PlaceType::Local, 1}, {PlaceType::Local, 2}}};
    auto add = testModule.buildFunction(funcCont);
    add->execute(context);
    EXPECT_EQ(x.get().get(), 12);

    context.set({PlaceType::Local, 3}, w);
    FunctionContext funcCont2{"get", FunctionOptions::Default, {PlaceType::Local, 3}, {{PlaceType::Local, 2}}};
    auto get = testModule.buildFunction(funcCont2);
    get->execute(context);
    EXPECT_EQ(w.get(), 2);
}

TEST_F(FunctionDefFixture, MethodWithJump)
{
    SpecTypeValueHolder<TestedClass> x, y, z;
    x.set({8});
    y.set({5});
    z.set({});
    ExecutionContext context;
    context.set({PlaceType::Local, 1}, x);
    context.set({PlaceType::Local, 2}, y);
    context.set({PlaceType::Local, 3}, z);

    CodeBlock code;
    FunctionContext funcCont{"==", FunctionOptions::Jump, {PlaceType::Void}, {{PlaceType::Local, 1}, {PlaceType::Local, 2}}, {1, 2}};
    code.operations.push_back(testModule.buildFunction(funcCont));
    funcCont.name = "add";
    funcCont.options = FunctionOptions::Default;
    funcCont.argPlaces = {{PlaceType::Local, 1}, {PlaceType::Local, 2}};
    code.operations.push_back(testModule.buildFunction(funcCont));
    funcCont.name = "set";
    funcCont.argPlaces = {{PlaceType::Local, 3}, {PlaceType::Local, 1}};
    code.operations.push_back(testModule.buildFunction(funcCont));

    context.run(code);

    EXPECT_EQ(z.get().get(), 8);
}

TEST_F(FunctionDefFixture, MethodWithJump2)
{
    SpecTypeValueHolder<TestedClass> x, y, z;
    x.set({5});
    y.set({5});
    z.set({});
    ExecutionContext context;
    context.set({PlaceType::Local, 1}, x);
    context.set({PlaceType::Local, 2}, y);
    context.set({PlaceType::Local, 3}, z);

    CodeBlock code;
    FunctionContext funcCont{"==", FunctionOptions::Jump, {PlaceType::Void}, {{PlaceType::Local, 1}, {PlaceType::Local, 2}}, {1, 2}};
    code.operations.push_back(testModule.buildFunction(funcCont));
    funcCont.name = "add";
    funcCont.options = FunctionOptions::Default;
    funcCont.argPlaces = {{PlaceType::Local, 1}, {PlaceType::Local, 2}};
    code.operations.push_back(testModule.buildFunction(funcCont));
    funcCont.name = "set";
    funcCont.argPlaces = {{PlaceType::Local, 3}, {PlaceType::Local, 1}};
    code.operations.push_back(testModule.buildFunction(funcCont));

    context.run(code);

    EXPECT_EQ(z.get().get(), 10);
}