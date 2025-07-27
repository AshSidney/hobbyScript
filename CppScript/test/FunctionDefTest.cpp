#include <gtest/gtest.h>

#include <CppScript/FunctionDef.h>
#include "TestUtils.h"

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

    Module testModule{"testModule", {}};
};

TEST_F(FunctionDefFixture, FunctionCreationAndExecution)
{
    DataBlockDef::Builder builder;
    const PlaceData place1{ PlaceType::Module, builder.addPlace(makeValue(10.0F)) };
    const PlaceData place2{ PlaceType::Module, builder.addPlace(makeValue(2.5F)) };
    const PlaceData place3{ PlaceType::Module, builder.addPlace(SpecTypeValueHolder<float>::specTypeId) };
 
    CodeBlock code{ { builder.build() } };
    ExecutionContext context;
    FunctionContext funcCont{"diff", {}, place3, {place1, place2}, {}, &code, &code.getDataLayout()};
    code.operations.push_back(testModule.buildFunction(funcCont));
    DataBlock<> data{code.getDataLayout()};
    context.run(code, data);
    EXPECT_EQ(static_cast<TypeValueHolder<float>&>(context.get(place3)).get(), 7.5F);
}

TEST_F(FunctionDefFixture, MethodDefConstructionAndCall)
{
    DataBlockDef::Builder builder;
    const PlaceData place1{ PlaceType::Module, builder.addPlace(makeValue(TestedClass{10})) };
    const PlaceData place2{ PlaceType::Module, builder.addPlace(makeValue(TestedClass{5})) };
    const PlaceData place3{ PlaceType::Module, builder.addPlace(makeValue(TestedClass{2})) };
    const PlaceData place4{ PlaceType::Module, builder.addPlace(makeValue(int(4))) };

    CodeBlock code{ { builder.build() } };
     ExecutionContext context;
    FunctionContext funcCont{"add", {}, {PlaceType::Void}, {place1, place3}, {}, &code, &code.getDataLayout()};
    code.operations.push_back(testModule.buildFunction(funcCont));
    FunctionContext funcCont2{"get", {}, place4, {place3}, {}, &code, &code.getDataLayout()};
    code.operations.push_back(testModule.buildFunction(funcCont2));

    DataBlock<> data{code.getDataLayout()};
    context.run(code, data);
    
    EXPECT_EQ(static_cast<TypeValueHolder<TestedClass>&>(context.get(place1)).get().get(), 12);
    EXPECT_EQ(static_cast<TypeValueHolder<int>&>(context.get(place4)).get(), 2);
}

TEST_F(FunctionDefFixture, MethodWithJump)
{
    DataBlockDef::Builder builder;
    const PlaceData place1{ PlaceType::Module, builder.addPlace(makeValue(TestedClass{8})) };
    const PlaceData place2{ PlaceType::Module, builder.addPlace(makeValue(TestedClass{5})) };
    const PlaceData place3{ PlaceType::Module, builder.addPlace(makeValue(TestedClass{})) };

    CodeBlock code{ { builder.build() } };
    ExecutionContext context;

    FunctionContext funcCont{"==", {FunctionOptions::Jump}, {PlaceType::Void}, {place1, place2}, {1, 2}, &code, &code.getDataLayout()};
    code.operations.push_back(testModule.buildFunction(funcCont));
    funcCont.name = "add";
    funcCont.options = {};
    funcCont.argPlaces = {place1, place2};
    code.operations.push_back(testModule.buildFunction(funcCont));
    funcCont.name = "set";
    funcCont.argPlaces = {place3, place1};
    code.operations.push_back(testModule.buildFunction(funcCont));

    DataBlock<> data{code.getDataLayout()};
    context.run(code, data);

    EXPECT_EQ(static_cast<TypeValueHolder<TestedClass>&>(context.get(place3)).get().get(), 8);
}

TEST_F(FunctionDefFixture, MethodWithJump2)
{
    DataBlockDef::Builder builder;
    const PlaceData place1{ PlaceType::Module, builder.addPlace(makeValue(TestedClass{5})) };
    const PlaceData place2{ PlaceType::Module, builder.addPlace(makeValue(TestedClass{5})) };
    const PlaceData place3{ PlaceType::Module, builder.addPlace(makeValue(TestedClass{})) };

    CodeBlock code{ { builder.build() } };
    ExecutionContext context;

    FunctionContext funcCont{"==", {FunctionOptions::Jump}, {PlaceType::Void}, {place1, place2}, {1, 2}, &code, &code.getDataLayout()};
    code.operations.push_back(testModule.buildFunction(funcCont));
    funcCont.name = "add";
    funcCont.options = {};
    funcCont.argPlaces = {place1, place2};
    code.operations.push_back(testModule.buildFunction(funcCont));
    funcCont.name = "set";
    funcCont.argPlaces = {place3, place1};
    code.operations.push_back(testModule.buildFunction(funcCont));

    DataBlock<> data{code.getDataLayout()};
    context.run(code, data);

    EXPECT_EQ(static_cast<TypeValueHolder<TestedClass>&>(context.get(place3)).get().get(), 10);
}