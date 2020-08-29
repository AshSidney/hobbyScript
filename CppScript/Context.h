#pragma once

#include <unordered_map>
#include <CppScript/Base.h>
#include <CppScript/TypeWrapper.h>
#include <CppScript/Operations.h>

namespace CppScript
{
	class Context
	{
	public:
		TypeBase::Ref get(const std::string& id);
		TypeBase::Ref set(const std::string& id, TypeBase::Ref value);

	private:
		std::unordered_map<std::string, TypeBase::Ref> data;
	};



	/*class ContextReader : public Visitable<ContextReader, OperationOld, ElementVisitor>
	{
	public:
		ContextReader() = default;
		ContextReader(const std::string& id) : elementId(id)
		{}

		void setUp(const Elements& data) override;
		TypeWrapperBase::Ref execute(Context& context) const override;

		//void accept(VisitorOld& visitor) const override;
		//void accept(VisitorOld& visitor) override;

	protected:
		std::string elementId;
	};*/

}