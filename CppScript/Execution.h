#pragma once

#include <CppScript/Operations.h>
#include <CppScript/Context.h>

namespace CppScript
{

class Executor
{
public:
	explicit Executor(Context& cntx);
	virtual ~Executor() = default;

	Context& getContext();

private:
	Context& context;
};


class ForLoopExecutor : public Executor
{
public:
	explicit ForLoopExecutor(Executor& parent);
};

}
