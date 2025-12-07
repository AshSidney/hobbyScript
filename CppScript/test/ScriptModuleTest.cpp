#include <gtest/gtest.h>

#include <CppScript/ScriptModule.h>
#include <CppScript/CoreModule.h>
#include <CppScript/IntValue.h>
#include "TestUtils.h"

using namespace CppScript;
using namespace CppScriptOld;

class ScriptModuleFixture : public testing::Test
{
protected:
    Module coreModule{createCoreModule()};
};

TEST_F(ScriptModuleFixture, ScriptModuleRun)
{
    DataBlockDef::Builder dataBuilder;
    const PlaceData placeVal1{ PlaceType::Module, dataBuilder.addPlace(makeValue(1234_I))};
    const PlaceData placeVal2{ PlaceType::Module, dataBuilder.addPlace(makeValue(5_I))};
    CodeBlock code{ dataBuilder.build() };
    FunctionContext funcCont{"+=", {}, {PlaceType::Void, 0}, {placeVal1, placeVal2}, {}, &code, &code.getDataLayout()};
    code.operations.push_back(coreModule.buildFunction(funcCont));

    ScriptModule testModule{ "testModule", std::move(code) };
    EXPECT_EQ(testModule.getData(), nullptr);

    ExecutionContext execCont;
    execCont.run(testModule);

    EXPECT_NE(testModule.getData(), nullptr);
    EXPECT_EQ(static_cast<TypeValueHolder<IntValue>&>(execCont.get(placeVal1)).get(), 1239_I);
    EXPECT_EQ(static_cast<TypeValueHolder<IntValue>&>(execCont.get(placeVal2)).get(), 5_I);

    execCont.run(testModule);
    
    EXPECT_NE(testModule.getData(), nullptr);
    EXPECT_EQ(static_cast<TypeValueHolder<IntValue>&>(execCont.get(placeVal1)).get(), 1239_I);
    EXPECT_EQ(static_cast<TypeValueHolder<IntValue>&>(execCont.get(placeVal2)).get(), 5_I);
}

TEST_F(ScriptModuleFixture, ScriptFunction)
{
    DataBlockDef::Builder builder;
    const PlaceData placeVal1{ PlaceType::Module, builder.addPlace(makeValue(1234_I))};
    const PlaceData placeVal2{ PlaceType::Module, builder.addPlace(makeValue(5_I))};
    CodeBlock code{ builder.build() };
    /*ScriptModule testModule{ "testModule" };
    FunctionContext funcCont{"+=", {}, {PlaceType::Void, 0}, {placeVal1, placeVal2}, {}, &code, &testModule};
    code.operations.push_back(coreModule.buildFunction(funcCont));
    ScriptFunctionDef scriptFnc{code, {}};
    testModule.defFunction("addModuleVars", std::move(scriptFnc));

    CodeBlock callCode{ {} };
    FunctionContext scriptCont{"addModuleVars", {}, {PlaceType::Void, 0}, {}, {}, &callCode, &testModule};
    ScriptFunction fncCall{testModule.buildFunction(scriptCont)};
    ExecutionContext execCont;
    execCont.run(testModule);
    fncCall.execute(execCont);

    EXPECT_EQ(static_cast<TypeValueHolder<IntValue>&>(execCont.get(placeVal1)).get(), 1239_I);
    EXPECT_EQ(static_cast<TypeValueHolder<IntValue>&>(execCont.get(placeVal2)).get(), 5_I);*/
}