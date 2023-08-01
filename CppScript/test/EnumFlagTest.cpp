#include <gtest/gtest.h>

#include <CppScript/EnumFlag.h>

const size_t sizeBits = sizeof(size_t) * 8;

enum class TestEnum
{
    First, Second, Third,
    NextVal = sizeBits + 4, NextNext,
    Last = 2 * sizeBits
};

namespace CppScript
{

template<> struct EnumTraits<TestEnum>
{
    static const TestEnum last{ TestEnum::Last };
};

}

using namespace CppScript;

TEST(EnumFlagTest, ConstructEvaluate)
{
    const EnumFlag testFlag1{ TestEnum::First };
    const EnumFlag testFlag2{ TestEnum::NextVal, TestEnum::Third };
    EXPECT_NE(testFlag1, testFlag2);
    EXPECT_EQ(testFlag1 & testFlag2, (EnumFlag<TestEnum>{}));
    const auto testFlag3 = testFlag1 | testFlag2;
    EXPECT_NE(testFlag1, testFlag3);
    EXPECT_NE(testFlag2, testFlag3);
    EXPECT_EQ(testFlag1 & testFlag3, testFlag1);
    EXPECT_EQ(testFlag2 & testFlag3, testFlag2);
    EXPECT_TRUE(testFlag3.contains(testFlag1));
    EXPECT_TRUE(testFlag3.contains(testFlag2));
    EXPECT_TRUE(testFlag3.contains(testFlag3));
    EXPECT_FALSE(testFlag1.contains(testFlag2));
    EXPECT_FALSE(testFlag1.contains(testFlag3));
}