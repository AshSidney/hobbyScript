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

void Context::swap(const std::string& id1, const std::string& id2)
{
	std::swap(data[id1], data[id2]);
}

}