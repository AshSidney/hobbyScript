#include <CppScript/Execution.h>


namespace CppScript
{

Executor::Executor(Context& cntx) : context(cntx)
{}

Context& Executor::getContext()
{
	return context;
}

/*Element::Ref ForLoop::execute() const
{

}*/

}