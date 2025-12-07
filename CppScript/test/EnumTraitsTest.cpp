#include <gmock/gmock.h>

#include <CppScript/EnumTraits.h>
#include "TestUtils.h"

const size_t sizeBits = sizeof(size_t) * 8;

enum class TestEnum
{
    First, Second, Third,
    NextVal = sizeBits + 4, NextNext,
    Last = 2 * sizeBits
};

enum class TestEnum2 { E1, E2 };

namespace CppScript
{

template <TestEnum V>
using TestItem = EnumItem<TestEnum, V>;

template<>
constexpr auto enumItems<TestEnum>()
{
    return std::make_tuple(Id{"TestEnum"},
        TestItem<TestEnum::First>{"first"}, TestItem<TestEnum::Second>{"second"},
        TestItem<TestEnum::NextNext>{"nn"}, TestItem<TestEnum::Last>{"last"},
        TestItem<TestEnum::Third>{"3rd"}, TestItem<TestEnum::NextVal>{"next"});
}

}

using namespace CppScript;
using namespace CppScriptTest;

static_assert(enumDefined<TestEnum>);
static_assert(!enumDefined<TestEnum2>);
static_assert(!enumDefined<int>);


TEST(EnumTraitsTest, EnumTraits)
{
    using TestTraits = EnumTraits<TestEnum>;

    EXPECT_EQ(TestTraits::id, Id{"TestEnum"});
    const auto& enumNames = TestTraits::itemNames;
    EXPECT_EQ(enumNames.size(), static_cast<size_t>(TestEnum::Last) + 1);
    EXPECT_EQ(enumNames[TestTraits::index(TestEnum::First)], "first");
    EXPECT_EQ(enumNames[TestTraits::index(TestEnum::Second)], "second");
    EXPECT_EQ(enumNames[TestTraits::index(TestEnum::Third)], "3rd");
    EXPECT_EQ(enumNames[TestTraits::index(TestEnum::NextVal)], "next");
    EXPECT_EQ(enumNames[TestTraits::index(TestEnum::NextNext)], "nn");
    EXPECT_EQ(enumNames[TestTraits::index(TestEnum::Last)], "last");

    EXPECT_EQ(Value<TestEnum>::typeId.typeName, Id{"TestEnum"});
    EXPECT_EQ(span2Array<TestTraits::size>(Value<TestEnum>::typeId.enumItems), enumNames);

    EXPECT_EQ(TestTraits::value(TestTraits::index(TestEnum::First)), TestEnum::First);
    EXPECT_EQ(TestTraits::value(TestTraits::index(TestEnum::Second)), TestEnum::Second);
    EXPECT_EQ(TestTraits::value(TestTraits::index(TestEnum::Third)), TestEnum::Third);
    EXPECT_EQ(TestTraits::value(TestTraits::index(TestEnum::NextVal)), TestEnum::NextVal);
    EXPECT_EQ(TestTraits::value(TestTraits::index(TestEnum::NextNext)), TestEnum::NextNext);
    EXPECT_EQ(TestTraits::value(TestTraits::index(TestEnum::Last)), TestEnum::Last);
}

TEST(EnumTraitsTest, ConstructEvaluate)
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