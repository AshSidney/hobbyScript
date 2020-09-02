#pragma once

#include <unordered_map>
#include <CppScript/Operations.h>

namespace CppScript
{
	class Context
	{
	public:
		TypeBase::Ref get(const std::string& id);
		TypeBase::Ref set(const std::string& id, TypeBase::Ref value);
		void swap(const std::string& id1, const std::string& id2);

	private:
		std::unordered_map<std::string, TypeBase::Ref> data;
	};

}