#pragma once

#include <CppScript/Operations.h>
#include <CppScript/Execution.h>

namespace CppScript
{

template <typename T> Operation::Ref CopyOperationBase::create(const Arguments& arguments)
{
	if (arguments.size() % 2 != 0)
		return {};
	for (size_t index = 0; index < arguments.size(); index += 2)
		if (arguments[index].typeId != &Type<T>::id())
			return {};
	auto operation = std::make_unique<CopyOperation<T>>();
	for (size_t index = 0; index < arguments.size(); index += 2)
		operation->addParams(arguments[index], arguments[index + 1]);
	return operation;
}


template <typename T> void CopyOperation<T>::execute(Executor& executor) const
{
	CopyOperationBase::execute(executor);
	for (const auto& param : copyParams)
		static_cast<Type<T>&>(*executor.get(param.target)).get() = static_cast<const Type<T>&>(*executor.get(param.source)).get();
}

template <typename T> void CopyOperation<T>::addParams(const Argument& source, const Argument& target)
{
	if (target.typeId != &Type<T>::id())
		CopyOperationBase::addParams(source, target);
	else
		copyParams.push_back({ source.place, target.place });
}

}
