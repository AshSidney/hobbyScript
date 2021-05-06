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

		virtual void serialize(std::vector<Operation::Ref>& objs, const char* name) = 0;
		virtual void serialize(Operation::Ref& obj) = 0;

		virtual void serialize(TypeBase::Ref& value, const char* name) = 0;
		virtual void serialize(IntValue& value, const char* name) = 0;
		virtual void serialize(std::string& value, const char* name) = 0;
	};


	class JsonLoader : public Serializer
	{
	public:
		JsonLoader(const Json& data);

		void serialize(std::vector<Operation::Ref>& objs, const char* name) override;
		void serialize(Operation::Ref& obj) override;

		void serialize(TypeBase::Ref& value, const char* name) override;
		void serialize(IntValue& value, const char* name) override;
		void serialize(std::string& value, const char* name) override;

	protected:
		//virtual const Json& getData();

	private:
		const Json& operationData;
	};
}