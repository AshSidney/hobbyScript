#include <gtest/gtest.h>

#include <CppScript/FunctionDef.h>

using namespace CppScript;


class TestedClass
{
public:
    int a;

    void add(const TestedClass& other)
    {
        a += other.a;
    }

    int get() const
    {
        return a;
    }
};

float diff(float x, float y)
{
    return x - y;
}

TEST(FunctionDefTest, FunctionCreationAndExecution)
{
    float x{10.0F};
    float y{2.5F};
    float r{0.0F};
    ExecutionContext context;
    context.set({MemoryType::Local, 1}, &x);
    context.set({MemoryType::Local, 2}, &y);
    context.set({MemoryType::Local, 3}, &r);

    FunctionDef difFunc{&diff};
    FunctionContext funcCont{FunctionSpec::Default};
    auto fnc = difFunc.createFunction(funcCont, {{MemoryType::Local, 3}, {MemoryType::Local, 1}, {MemoryType::Local, 2}});
    fnc->execute(context);
    EXPECT_EQ(r, 7.5F);
}

TEST(FunctionDefTest, MethodDefConstructionAndCall)
{
    FunctionDef addMethod{&TestedClass::add};
    FunctionDef getMethod{&TestedClass::get};
    
    TestedClass x{10};
    TestedClass y{5};
    TestedClass z{2};
    int w{0};

    ExecutionContext context;
    context.set({MemoryType::Local, 1}, &x);
    context.set({MemoryType::Local, 2}, &z);
    FunctionContext funcCont{FunctionSpec::ReturnVoid};
    auto add = addMethod.createFunction(funcCont, {{MemoryType::Local, 1}, {MemoryType::Local, 2}});
    add->execute(context);
    EXPECT_EQ(x.get(), 12);

    context.set({MemoryType::Local, 3}, &w);
    FunctionContext funcCont2{FunctionSpec::Default};
    auto getC = getMethod.createFunction(funcCont2, {{MemoryType::Local, 3}, {MemoryType::Local, 2}});
    getC->execute(context);
    EXPECT_EQ(w, 2);
}

/*TEST(FunctionDefTest, FunctionDefConstructionAndCall)
{
    FunctionDef diffFunc{&diff};

    float a { 23.0F };
    float b { 11.25F };
    float c { -4.25F };

    ExecutionContext context;
    context.set({MemoryType::Local, 0}, &a);
    context.set({MemoryType::Local, 1}, &b);
    context.set({MemoryType::Local, 2}, &c);
    auto df = diffFunc.createFunction(FunctionSpec::Default, {{MemoryType::Local, 0}, {MemoryType::Local, 1}, {MemoryType::Local, 2}});
    df->execute(context);
    EXPECT_EQ(a, 15.5F);
}*/