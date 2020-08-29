#pragma once

#include <string>

namespace CppScript
{

	template <typename T> struct TypeNameHelper
	{
		static std::string getTypeName()
		{
			constexpr auto prefixSize = sizeof("CppScript::TypeNameHelper<") - 1;
			constexpr auto postfixSize = sizeof(">::getTypeName");
			return { __FUNCTION__, prefixSize, sizeof(__FUNCTION__) - prefixSize - postfixSize };
		}
	};

	template <typename T> std::string getTypeName()
	{
		return TypeNameHelper<T>::getTypeName();
	};

}