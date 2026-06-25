#include <CppScript/ParserUtils.h>
#include "ParserUtilsTest.h"
#include <gmock/gmock.h>
#include <bit>

namespace CppScriptTest
{

HashParamsCalculator::HashParamsCalculator(unsigned short maxSize)
    : maxHashSize(maxSize)
{}

std::optional<HashParamsCalculator::HashParams> HashParamsCalculator::calculate(const std::vector<std::string_view>& tokens)
{
	const unsigned short initHashSize{ 1u << std::bit_width(tokens.size() - 1) };
	CalcState state{ initHashSize, getMinMaxChars(tokens) };
	do
	{
	    HashParams params;
		params.setUp(state);
		do
		{
			if (params.validate(tokens))
			    return params;
		}
		while (params.nextState());
	}
	while (state.nextState(maxHashSize));
    return {};
}

std::pair<unsigned short, unsigned short> HashParamsCalculator::getMinMaxChars(const std::vector<std::string_view>& tokens)
{
    std::pair<unsigned short, unsigned short> minMax{ 0, 0 };
    for (std::size_t index = 0; index < tokens.size(); ++index)
    {
        minMax.second = std::max(minMax.second, static_cast<unsigned short>(tokens[index].size()));
        if (index > 0)
        {
            const std::size_t commonSize = std::min(tokens[index - 1].size(), tokens[index].size());
            std::size_t charIndex = 0;
            for (; charIndex < commonSize; ++charIndex)
                if (tokens[index - 1][charIndex] != tokens[index][charIndex])
                    break;
            minMax.first = std::max(minMax.first, static_cast<unsigned short>(charIndex));
        } 
    }
    return minMax;
}


HashParamsCalculator::CalcState::CalcState(unsigned short initHashSize, std::pair<unsigned short, unsigned short> minMaxChars)
    : hashSize(initHashSize), minChars(minMaxChars.first + 1), maxChars(minMaxChars.second)
{
	setMaxRepeat();
    shiftRepeats.resize(minChars);
    fillShifts(0, minChars);
}

bool HashParamsCalculator::CalcState::nextState(const unsigned short maxHashSize)
{
    unsigned short shiftsCount = 0;
    for (unsigned short index = 0; index < shiftRepeats.size(); ++index)
    {
        unsigned short& currRepeat = shiftRepeats[shiftRepeats.size() - 1 - index];
        if (currRepeat > 0 && shiftsCount < index * maxRepeat)
        {
            --currRepeat;
            fillShifts(static_cast<unsigned short>(shiftRepeats.size() - index), shiftsCount + 1);
            return true;
        }
        shiftsCount += currRepeat;
    }
	if (shiftsCount < shiftRepeats.size() * maxRepeat)
	{
		fillShifts(0, shiftsCount + 1);
		return true;
	}
	if (shiftRepeats.size() < maxChars)
	{
		shiftRepeats.push_back(0);
		fillShifts(0, static_cast<unsigned short>(shiftRepeats.size()));
		return true;
	}
    hashSize <<= 1;
	setMaxRepeat();
    fillShifts(0, minChars);
    return hashSize <= maxHashSize;
}

void HashParamsCalculator::CalcState::setMaxRepeat()
{
    maxRepeat = std::bit_width(hashSize) - 1;
}

void HashParamsCalculator::CalcState::fillShifts(unsigned short shiftIndex, unsigned short shiftsCount)
{
    for (; shiftIndex < shiftRepeats.size(); ++shiftIndex)
    {
        shiftRepeats[shiftIndex] = std::min(shiftsCount, maxRepeat);
        shiftsCount -= shiftRepeats[shiftIndex];
    }
 }


void HashParamsCalculator::HashParams::setUp(const CalcState& state)
{
	mask = state.hashSize - 1;
    shifts.clear();
    unsigned short index = 0;
    for (unsigned short repeat : state.shiftRepeats)
        shifts.insert(shifts.end(), repeat, { index++, 0 });
}
        
unsigned short HashParamsCalculator::HashParams::calculate(const std::string_view token) const
{
    unsigned short hash{ 0 };
    for (const CharShift shift : shifts)
    {
        if (shift.index >= token.size())
            break;
        hash ^= token[shift.index] << shift.shift;
    }
    return (hash >> maskShift) & mask;
}

bool HashParamsCalculator::HashParams::validate(const std::vector<std::string_view>& tokens) const
{
    std::vector<bool> hasHash(mask + 1, false);
    for (std::string_view token : tokens)
    {
        const auto hash = calculate(token);
        if (hasHash[hash])
            return false;
        hasHash[hash] = true;
    }
    return true;
}

bool HashParamsCalculator::HashParams::nextState()
{
    if (++maskShift < maxShiftValue)
        return true;
    maskShift = 0;
    for (std::size_t index = 0; index < shifts.size(); ++index)
    {
        auto& currShift = shifts[shifts.size() - 1 - index].shift;
        if (++currShift < maxShiftValue)
            return true;
        currShift = 0;
    }
    return false;
}


std::ostream & operator<<(std::ostream& os, const HashParamsCalculator::HashParams& params)
{
    os << "Mask: " << params.mask << ", MaskShift: " << params.maskShift << ", Shifts: [";
    for (std::size_t index = 0; index < params.shifts.size(); ++index)
    {
        if (index > 0)
            os << ", ";
        os << "{Index: " << params.shifts[index].index << ", Shift: " << params.shifts[index].shift << "}";
    }
    os << "]";
    return os;
}

}


using namespace CppScript;


TEST(TokenMapTest, BasicFunctionality)
{
    auto hashFunc = [](std::string_view token) -> unsigned short
    {
        if (token == "one") return 0;
        if (token == "two") return 1;
        if (token == "three") return 2;
        if (token == "four") return 3;
        return 4;
    };

    static constexpr TokenMap<int, 8, hashFunc> tokenMap
        {
            {"one", 1},
            {"two", 2},
            {"three", 3},
            {"four", 4},
            {"five", 5}
        };

    EXPECT_EQ(tokenMap["one"], 1);
    EXPECT_EQ(tokenMap["two"], 2);
    EXPECT_EQ(tokenMap["three"], 3);
    EXPECT_EQ(tokenMap["four"], 4);
    EXPECT_EQ(tokenMap["five"], 5);
}