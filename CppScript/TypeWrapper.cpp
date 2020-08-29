#include "TypeWrapper.h"
#include <sstream>

namespace CppScript
{

TypeDescriptorBase::TypeDescriptorBase(std::string&& descr) noexcept : description(std::move(descr))
{}

const std::string& TypeDescriptorBase::getDescription() const noexcept
{
	return description;
}


InvalidTypeCastOld::InvalidTypeCastOld(const std::string& classFrom, const std::string& classTo) noexcept
	: fromClassName(classFrom), toClassName(classTo)
{
	std::ostringstream messageStream;
	messageStream << "Cannot cast from: " << fromClassName << " to: " << toClassName;
	message = messageStream.str();
}

const char* InvalidTypeCastOld::what() const noexcept
{
	return message.c_str();
}

}
