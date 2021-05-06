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


JsonLoader::JsonLoader(const Json& data) : operationData(data)
{}

void JsonLoader::serialize(Operation::Ref& obj)
{
	obj = Operation::create(operationData["type"].get<std::string>());
	if (obj)
	{
		obj->serialize(*this);
	}
}

void JsonLoader::serialize(std::vector<Operation::Ref>& objs, const char* name)
{
	const auto& data = operationData[name];
	if (!data.is_array())
		return;
	for (const auto& objData : data)
	{
		JsonLoader opLoader{ objData };
		Operation::Ref obj;
		opLoader.serialize(obj);
		objs.push_back(std::move(obj));
	}
}

void JsonLoader::serialize(TypeBase::Ref& value, const char* name)
{
	const auto& data = operationData[name];
	if (data.is_number_integer())
		value = TypeInt::create(data.get<TypeInt::ValueType>());
	else if (data.is_number_float())
		value = TypeFloat::create(data.get<TypeFloat::ValueType>());
	else if (data.is_boolean())
		value = data.get<bool>() ? TypeBool::trueValue : TypeBool::falseValue;
	else
		throw NotBaseType{ data.type_name() };
}

void JsonLoader::serialize(IntValue& value, const char* name)
{
	value = operationData[name].get<IntValue>();
}

void JsonLoader::serialize(std::string& value, const char* name)
{
	value = operationData[name].get<std::string>();
}

}