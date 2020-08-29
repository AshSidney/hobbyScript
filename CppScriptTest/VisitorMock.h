#pragma once

#include <CppScript/Base.h>
#include <CppScript/Operations.h>
#include <gmock/gmock.h>

namespace CppScript
{

	class VisitorMock : public ElementVisitorFailing
	{
		/*MOCK_METHOD1(visit, void(const IntValue& value));
		MOCK_METHOD1(visit, void(const BoolValue& value));
		MOCK_METHOD1(visit, void(const FloatValue& value));
		MOCK_METHOD1(visit, void(const StringValue& value));*/
		MOCK_METHOD1(visit, void(const OperationOld& operation));
	};

}
