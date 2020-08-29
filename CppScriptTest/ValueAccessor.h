#pragma once

#include <CppScript/Base.h>
#include <CppScript/Operations.h>

namespace CppScript
{
	class ValueAccessor : public ElementVisitorFailing
	{
	public:
		virtual void visit(const TypeWrapperBase& wrapper) override
		{
			intValue = TypeWrapper<long long>::typeDescriptor.get(wrapper);
		}
		virtual void visit(TypeWrapperBase& wrapper) override
		{
			const TypeWrapperBase& constWrapper{ wrapper };
			visit(constWrapper);
		}

		long long intValue{ 0 };
		bool boolValue{ false };
		double floatValue{ 0.0 };
		std::string stringValue;
	};

}
