#include <gtest/gtest.h>
#include <CppScript/TypeInfo.h>

using namespace CppScript;

class FirstToTest
{};

class SecName
{};

TEST(TypeInfoTestCase, GetTypeName)
{
	EXPECT_EQ(getTypeName<int>(), "int");
	EXPECT_EQ(getTypeName<float>(), "float");
	EXPECT_EQ(getTypeName<FirstToTest>(), "class FirstToTest");
	EXPECT_EQ(getTypeName<SecName>(), "class SecName");
}