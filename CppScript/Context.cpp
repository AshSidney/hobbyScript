#include <CppScript/Context.h>

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


}