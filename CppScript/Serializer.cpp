#include <CppScript/Serializer.h>
#include <CppScript/BasicTypes.h>

namespace CppScript
{

class NotBaseType : public std::exception
{
public:
	NotBaseType(const char* typeName) noexcept : typeName(typeName)
	{
		std::ostringstream messageStream;
		messageStream << "Type: " << typeName << " is not a basic type, therefore cannot be serialized";
		message = messageStream.str();
	}

	virtual const char* what() const noexcept
	{
		return message.c_str();
	}

private:
	const char* typeName;
	std::string message;
};


class JsonArrayLoader : public JsonLoader
{
public:
	explicit JsonArrayLoader(const Json& data) : JsonLoader(data), currentData(data.begin())
	{}

protected:
	const Json& getData() override
	{
		return *currentData++;
	}

private:
	Json::const_iterator currentData;
};


JsonLoader::JsonLoader(const Json& data) : operationData(data)
{}

void JsonLoader::serialize(Operation::Ref& obj)
{
	const auto& data = getData();
	obj = Operation::create(data["type"].get<std::string>());
	if (obj)
	{
		const auto& opData = data["data"];
		if (opData.is_array())
		{
			JsonArrayLoader opLoader{ opData };
			obj->serialize(opLoader);
		}
		else
		{
			JsonLoader opLoader{ opData };
			obj->serialize(opLoader);
		}
	}
}

void JsonLoader::serialize(TypeBase::Ref& value)
{
	const auto& data = getData();
	if (data.is_number_integer())
		value = TypeInt::create(data.get<TypeInt::ValueType>());
	else if (data.is_number_float())
		value = TypeFloat::create(data.get<TypeFloat::ValueType>());
	else if (data.is_boolean())
		value = data.get<bool>() ? TypeBool::trueValue : TypeBool::falseValue;
	else
		throw NotBaseType{ data.type_name() };
}

void JsonLoader::serialize(std::string& value)
{
	value = getData().get<std::string>();
}

const Json& JsonLoader::getData()
{
	return operationData;
}

}