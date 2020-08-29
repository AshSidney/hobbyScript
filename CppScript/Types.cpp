#include <CppScript/Types.h>

namespace CppScript
{

class InvalidOperation : public std::exception
{
public:
	InvalidOperation(const char* typeName, const char* opName) noexcept;

	virtual const char* what() const noexcept override;

private:
	const char* typeName;
	const char* operationName;
	std::string message;
};


TypeBase::Ref TypeBase::clone() const
{
	throw InvalidOperation{ getId().getName(), "Cloning" };
}

TypeBase::Ref TypeBase::operator+=(const TypeBase& obj)
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


	InvalidOperation::InvalidOperation(const char* typeName, const char* opName) noexcept
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


	InvalidTypeCast::InvalidTypeCast(const char* fromType, const char* toType) noexcept
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