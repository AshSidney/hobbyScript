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
    CodeBlock code;
    const auto place1 = code.addPlaceData(x.getTypeId());
    const auto place2 = code.addPlaceData(y.getTypeId());
    const auto place3 = code.addPlaceData(r.getTypeId());
    ExecutionContext context;
    context.set(place1, x);
    context.set(place2, y);
    context.set(place3, r);

    FunctionContext funcCont{"diff", FunctionOptions::Default, place3, {place1, place2}, {}, &code};
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
    CodeBlock code;
    const auto place1 = code.addPlaceData(x.getTypeId());
    const auto place2 = code.addPlaceData(y.getTypeId());
    const auto place3 = code.addPlaceData(z.getTypeId());
    SpecTypeValueHolder<int> w;
    const auto place4 = code.addPlaceData(w.getTypeId());

    ExecutionContext context;
    context.set(place1, x);
    context.set(place3, z);
    FunctionContext funcCont{"add", FunctionOptions::Default, {PlaceType::Void}, {place1, place3}, {}, &code};
    auto add = testModule.buildFunction(funcCont);
    add->execute(context);
    EXPECT_EQ(x.get().get(), 12);

    context.set(place4, w);
    FunctionContext funcCont2{"get", FunctionOptions::Default, place4, {place3}, {}, &code};
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
    CodeBlock code;
    const auto place1 = code.addPlaceData(x.getTypeId());
    const auto place2 = code.addPlaceData(y.getTypeId());
    const auto place3 = code.addPlaceData(z.getTypeId());
    ExecutionContext context;
    context.set(place1, x);
    context.set(place2, y);
    context.set(place3, z);

    FunctionContext funcCont{"==", FunctionOptions::Jump, {PlaceType::Void}, {place1, place2}, {1, 2}, &code};
    code.operations.push_back(testModule.buildFunction(funcCont));
    funcCont.name = "add";
    funcCont.options = FunctionOptions::Default;
    funcCont.argPlaces = {place1, place2};
    code.operations.push_back(testModule.buildFunction(funcCont));
    funcCont.name = "set";
    funcCont.argPlaces = {place3, place1};
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
    CodeBlock code;
    const auto place1 = code.addPlaceData(x.getTypeId());
    const auto place2 = code.addPlaceData(y.getTypeId());
    const auto place3 = code.addPlaceData(z.getTypeId());
    ExecutionContext context;
    context.set(place1, x);
    context.set(place2, y);
    context.set(place3, z);

    FunctionContext funcCont{"==", FunctionOptions::Jump, {PlaceType::Void}, {place1, place2}, {1, 2}, &code};
    code.operations.push_back(testModule.buildFunction(funcCont));
    funcCont.name = "add";
    funcCont.options = FunctionOptions::Default;
    funcCont.argPlaces = {place1, place2};
    code.operations.push_back(testModule.buildFunction(funcCont));
    funcCont.name = "set";
    funcCont.argPlaces = {place3, place1};
    code.operations.push_back(testModule.buildFunction(funcCont));

    context.run(code);

    EXPECT_EQ(z.get().get(), 10);
}