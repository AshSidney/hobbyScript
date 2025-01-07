#include <CppScript/ScriptModule.h>

namespace CppScript
{

ScriptFunction::ScriptFunction(const CodeBlock& code, DataBlockDef::Layout data)
    : code(code), dataLayout(data)
{}

void ScriptFunction::execute(ExecutionContext& context) const
{
    
}


ScriptModule::ScriptModule(std::string_view name, CodeBlock code)
    : Module(name), code(std::move(code))
{}

}
