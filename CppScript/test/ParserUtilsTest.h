#pragma once

#include <CppScript/ParserUtils.h>
#include <vector>
#include <optional>
#include <iostream>

namespace CppScriptTest
{

class HashParamsCalculator
{
public:
    HashParamsCalculator(unsigned short maxSize);
    
    struct CalcState
    {
        CalcState(unsigned short initHashSize, std::pair<unsigned short, unsigned short> minMaxChars);

        bool nextState(unsigned short maxHashSize);

        void setMaxRepeat();
        void fillShifts(unsigned short shiftIndex, unsigned short shiftsCount);
        
    	unsigned short hashSize{ 0 };
        unsigned short maxRepeat{ 0 };
		unsigned short minChars{ 0 };
        unsigned short maxChars{ 0 };
    	std::vector<unsigned short> shiftRepeats;
    };

	struct CharShift
    {
        unsigned short index{ 0 };
        unsigned short shift{ 0 };
    };
    
    struct HashParams
    {
        void setUp(const CalcState& state);

        unsigned short calculate(const std::string_view token) const;
        bool validate(const std::vector<std::string_view>& tokens) const;
        
        bool nextState();

    	unsigned short mask{ 0 };
    	unsigned short maskShift{ 0 };
        std::vector<CharShift> shifts;

        static constexpr unsigned short maxShiftValue{ 8 };
    };
    
    std::optional<HashParams> calculate(const std::vector<std::string_view>& tokens);

private:
    static HashParams createParams(const CalcState& state);
    
    static std::pair<unsigned short, unsigned short> getMinMaxChars(const std::vector<std::string_view>& tokens);
    
    unsigned short maxHashSize;
};


std::ostream & operator<<(std::ostream& os, const HashParamsCalculator::HashParams& params);


template<typename TM>
class TestTokenMap : public TM
{
public:
    explicit TestTokenMap(const TM& tokenMap) : TM(tokenMap)
    {
        std::sort(TM::tokens.begin(), TM::tokens.end());
        for (const auto& token : TM::tokens)
            sortedTokens.push_back(token);
    }

    bool validateHashFunction() const
    {
        std::array<bool, TM::hashSize> checkUniqueHash{ false };
        const bool uniqueHash = std::all_of(sortedTokens.begin(), sortedTokens.end(),
            [this, &checkUniqueHash](const std::string_view& token)
            {
                const auto hash = TM::calculateHash(token);
                if (hash >= TM::hashSize || checkUniqueHash[hash])
                    return false;
                checkUniqueHash[hash] = true;
                return true;
            });
        if (!uniqueHash)
        {
            HashParamsCalculator hashCalc{ 0x100 };
            auto hashParams = hashCalc.calculate(sortedTokens);
            if (hashParams.has_value())
                std::cout << "Hash function validation failed. Create hash function with suggested parameters:" << std::endl
                    << *hashParams << std::endl;
            else
                std::cout << "Hash function validation failed. Not able to provide suggestion." << std::endl;
        }
        return uniqueHash;
    }

    std::vector<std::string_view> sortedTokens;
};

}
