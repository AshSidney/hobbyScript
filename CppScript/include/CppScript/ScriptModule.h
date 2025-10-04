#pragma once

#include <CppScript/Definitions.h>
#include <CppScript/FunctionDef.h>

namespace CppScriptOld
{

class CPPSCRIPT_API ScriptFunction : public Function
{
public:
    ScriptFunction(const CodeBlock& code, DataBlockDef::Layout data);

    void execute(ExecutionContext& context) const override;

    DataBlock<>& getModuleData() const;

private:
    const CodeBlock& code;
    DataBlockDef::Layout dataLayout;
    mutable DataBlock<>* moduleData{ nullptr };
};

class CPPSCRIPT_API ScriptFunctionDef : public FunctionDef
{
public:
    std::unique_ptr<Function> buildFunction(FunctionContext& context) const override;

};

class CPPSCRIPT_API ScriptModule : public Module
{
public:
    ScriptModule(std::string_view name, CodeBlock code);

    const CodeBlock& getCode() const
    {
        return code;
    }

    DataBlock<>* getData() const
    {
        return data;
    }

    void setData(DataBlock<>& dataBlock) const
    {
        data = &dataBlock;
    }

private:
    CodeBlock code;
    mutable DataBlock<>* data{ nullptr };
};

}