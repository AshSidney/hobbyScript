#include <CppScript/Context.h>
#include <CppScript/TypeWrapper.h>

namespace CppScript
{

TypeBase::Ref Context::get(const std::string& id)
{
	return data.at(id);
}

TypeBase::Ref Context::set(const std::string& id, TypeBase::Ref value)
{
	return data[id] = value;
}


/*class ElementsExtractor : public VisitorOld
{
public:
	ElementsExtractor
};
void ContextReader::setUp(const Elements& data)
{
	elementId = getValue<std::string>(data.elements[0]);
}

TypeWrapperBase::Ref ContextReader::execute(Context& context) const
{
	return context[elementId];
}*/

/*void ContextReader::accept(VisitorOld& visitor) const
{
	//visitor.visit(*this);
}
void ContextReader::accept(VisitorOld& visitor)
{
	//visitor.visit(*this);
}*/


}