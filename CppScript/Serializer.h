#pragma once

#include <CppScript/Types.h>
#include <CppScript/Operations.h>
#include <CppScript/Json.h>

namespace CppScript
{

	class Serializer
	{
	public:
		virtual ~Serializer() = default;

		virtual void serialize(Operation::Ref& obj) = 0;

		virtual void serialize(TypeBase::Ref& value) = 0;
		virtual void serialize(std::string& value) = 0;
	};


	class JsonLoader : public Serializer
	{
	public:
		JsonLoader(const Json& data);

		virtual void serialize(Operation::Ref& obj) override;

		virtual void serialize(TypeBase::Ref& value) override;
		virtual void serialize(std::string& value) override;

	protected:
		virtual const Json& getData();

	private:
		const Json& operationData;
	};
}