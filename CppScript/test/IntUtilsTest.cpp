#include <IntUtils.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>


std::vector<unsigned int> collectDigits(CppScript::IntParser& parser)
{
    std::vector<unsigned int> digits;
    std::optional<unsigned int> digit;
    while(digit = parser.getNextDigit())
        digits.push_back(*digit);
    return digits;
}

namespace CppScript
{
    bool operator==(const CppScript::IntParser::NumberSystem& l, const CppScript::IntParser::NumberSystem& r)
    {
        return l.base == r.base && l.shift == r.shift;
    }
}

const CppScript::IntParser::NumberSystem binNS{2, 1};
const CppScript::IntParser::NumberSystem octNS{8, 3};
const CppScript::IntParser::NumberSystem decNS{10, 0};
const CppScript::IntParser::NumberSystem hexNS{16, 4};
const CppScript::IntParser::NumberSystem invNS{0, 0};


TEST(IntParserTest, ParseValidIntLiterals)
{
    CppScript::IntParser parserDec{"45_612_3"};
    EXPECT_TRUE(parserDec.isValid());
    EXPECT_TRUE(parserDec.isPositive());
    EXPECT_EQ(parserDec.getNumberSystem(), decNS);
    EXPECT_THAT(collectDigits(parserDec), testing::ElementsAre(4, 5, 6, 1, 2, 3));
    EXPECT_TRUE(parserDec.hasFinished());
    EXPECT_TRUE(parserDec.isValid());

    CppScript::IntParser parserHex{"-0X8a9_bc"};
    EXPECT_TRUE(parserHex.isValid());
    EXPECT_FALSE(parserHex.isPositive());
    EXPECT_EQ(parserHex.getNumberSystem(), hexNS);
    EXPECT_THAT(collectDigits(parserHex), testing::ElementsAre(8, 10, 9, 11, 12));
    EXPECT_TRUE(parserHex.hasFinished());
    EXPECT_TRUE(parserHex.isValid());

    CppScript::IntParser parserOct{"+0o_7_24"};
    EXPECT_TRUE(parserOct.isValid());
    EXPECT_TRUE(parserOct.isPositive());
    EXPECT_EQ(parserOct.getNumberSystem(), octNS);
    EXPECT_THAT(collectDigits(parserOct), testing::ElementsAre(7, 2, 4));
    EXPECT_TRUE(parserOct.hasFinished());
    EXPECT_TRUE(parserOct.isValid());

    CppScript::IntParser parserBin{"-0b0_11_01"};
    EXPECT_TRUE(parserBin.isValid());
    EXPECT_FALSE(parserBin.isPositive());
    EXPECT_EQ(parserBin.getNumberSystem(), binNS);
    EXPECT_THAT(collectDigits(parserBin), testing::ElementsAre(0, 1, 1, 0, 1));
    EXPECT_TRUE(parserBin.hasFinished());
    EXPECT_TRUE(parserBin.isValid());

    CppScript::IntParser parserDecCppDelim{"-78'932'10"};
    EXPECT_TRUE(parserDecCppDelim.isValid());
    EXPECT_FALSE(parserDecCppDelim.isPositive());
    EXPECT_EQ(parserDecCppDelim.getNumberSystem(), decNS);
    EXPECT_THAT(collectDigits(parserDecCppDelim), testing::ElementsAre(7, 8, 9, 3, 2, 1, 0));
    EXPECT_TRUE(parserDecCppDelim.hasFinished());
    EXPECT_TRUE(parserDecCppDelim.isValid());
}

TEST(IntParserTest, ParseInvalidIntLiterals)
{
    CppScript::IntParser emptyParser{""};
    EXPECT_FALSE(emptyParser.isValid());
    EXPECT_EQ(emptyParser.getNumberSystem(), invNS);
    EXPECT_TRUE(collectDigits(emptyParser).empty());
    EXPECT_TRUE(emptyParser.hasFinished());

    CppScript::IntParser invalidParserStartDelim{"_234"};
    EXPECT_FALSE(invalidParserStartDelim.isValid());
    EXPECT_EQ(invalidParserStartDelim.getNumberSystem(), invNS);
    EXPECT_TRUE(collectDigits(invalidParserStartDelim).empty());
    EXPECT_FALSE(invalidParserStartDelim.hasFinished());

    CppScript::IntParser invalidParser2Delim{"0O2__34"};
    EXPECT_TRUE(invalidParser2Delim.isValid());
    EXPECT_EQ(invalidParser2Delim.getNumberSystem(), octNS);
    EXPECT_THAT(collectDigits(invalidParser2Delim), testing::ElementsAre(2));
    EXPECT_FALSE(invalidParser2Delim.hasFinished());
    EXPECT_FALSE(invalidParser2Delim.isValid());

    CppScript::IntParser invalidParserEndDelim{"0x_abc_"};
    EXPECT_TRUE(invalidParserEndDelim.isValid());
    EXPECT_EQ(invalidParserEndDelim.getNumberSystem(), hexNS);
    EXPECT_THAT(collectDigits(invalidParserEndDelim), testing::ElementsAre(10, 11, 12));
    EXPECT_TRUE(invalidParserEndDelim.hasFinished());
    EXPECT_FALSE(invalidParserEndDelim.isValid());

    CppScript::IntParser invalidParserDiffDelims{"5_472'69'"};
    EXPECT_TRUE(invalidParserDiffDelims.isValid());
    EXPECT_EQ(invalidParserDiffDelims.getNumberSystem(), decNS);
    EXPECT_THAT(collectDigits(invalidParserDiffDelims), testing::ElementsAre(5, 4, 7, 2));
    EXPECT_FALSE(invalidParserDiffDelims.hasFinished());
    EXPECT_FALSE(invalidParserDiffDelims.isValid());

    CppScript::IntParser invalidParserBin{"0B10_120"};
    EXPECT_TRUE(invalidParserBin.isValid());
    EXPECT_EQ(invalidParserBin.getNumberSystem(), binNS);
    EXPECT_THAT(collectDigits(invalidParserBin), testing::ElementsAre(1, 0, 1));
    EXPECT_FALSE(invalidParserBin.hasFinished());
    EXPECT_FALSE(invalidParserBin.isValid());

    CppScript::IntParser invalidParserOct{"-0o789"};
    EXPECT_TRUE(invalidParserOct.isValid());
    EXPECT_EQ(invalidParserOct.getNumberSystem(), octNS);
    EXPECT_THAT(collectDigits(invalidParserOct), testing::ElementsAre(7));
    EXPECT_FALSE(invalidParserOct.hasFinished());
    EXPECT_FALSE(invalidParserOct.isValid());

    CppScript::IntParser invalidParserDec{"+78x9"};
    EXPECT_TRUE(invalidParserDec.isValid());
    EXPECT_EQ(invalidParserDec.getNumberSystem(), decNS);
    EXPECT_THAT(collectDigits(invalidParserDec), testing::ElementsAre(7, 8));
    EXPECT_FALSE(invalidParserDec.hasFinished());
    EXPECT_FALSE(invalidParserDec.isValid());

    CppScript::IntParser invalidParserHex{"0xefgh"};
    EXPECT_TRUE(invalidParserHex.isValid());
    EXPECT_EQ(invalidParserHex.getNumberSystem(), hexNS);
    EXPECT_THAT(collectDigits(invalidParserHex), testing::ElementsAre(14, 15));
    EXPECT_FALSE(invalidParserHex.hasFinished());
    EXPECT_FALSE(invalidParserHex.isValid());
}