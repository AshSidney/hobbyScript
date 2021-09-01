#include <CppScript/Types.h>

namespace CppScript
{

std::unique_ptr<std::unordered_map<std::string_view, const TypeIdBase*>> TypeIdBase::types;


TypeIdBase::TypeIdBase(const std::string_view name) noexcept : typeName(name)
{
	if (!types)
		types = std::make_unique<std::unordered_map<std::string_view, const TypeIdBase*>>();
	(*types)[typeName] = this;
}



class InvalidOperation : public std::exception
{
public:
	InvalidOperation(const std::string_view& typeName, const char* opName) noexcept;

	virtual const char* what() const noexcept override;

private:
	std::string_view typeName;
	const char* operationName;
	std::string message;
};


TypeBase::Ref TypeBase::clone() const
{
	throw InvalidOperation{ getId().getName(), "Cloning" };
}

void TypeBase::operator+=(const TypeBase& obj)
{
	throw InvalidOperation{ getId().getName(), "Addition" };
}

bool TypeBase::operator==(const TypeBase& obj) const
{
	throw InvalidOperation{ getId().getName(), "Equals" };
}

bool TypeBase::operator<(const TypeBase& obj) const
{
	throw InvalidOperation{ getId().getName(), "Less" };
}


InvalidOperation::InvalidOperation(const std::string_view& typeName, const char* opName) noexcept
	: typeName(typeName), operationName(opName)
{
	std::ostringstream messageStream;
	messageStream << "Type: " << typeName << ": " << operationName << " is not supported";
	message = messageStream.str();
}

const char* InvalidOperation::what() const noexcept
{
	return message.c_str();
}


InvalidTypeCast::InvalidTypeCast(const std::string_view& fromType, const std::string_view& toType) noexcept
	: fromTypeName(fromType), toTypeName(toType)
{
	std::ostringstream messageStream;
	messageStream << "Cannot cast from: " << fromTypeName << " to: " << toTypeName;
	message = messageStream.str();
}

const char* InvalidTypeCast::what() const noexcept
{
	return message.c_str();
}

}