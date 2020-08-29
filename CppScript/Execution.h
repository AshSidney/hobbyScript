#pragma once

#include <CppScript/Base.h>
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


class ForLoopExecuter : public Executor
{
public:
	explicit ForLoopExecuter(Executor& parent);
	/*void accept(VisitorOld& visitor) override
	{
		visitor.visit(*this);
	}*/

	//Element::Ref execute() const override;

protected:

	//Iterator& iterator;
};

}
