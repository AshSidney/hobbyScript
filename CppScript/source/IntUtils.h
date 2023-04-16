#pragma once

#include <CppScript/Definitions.h>

#include <string_view>
#include <optional>

namespace CppScript
{

class CPPSCRIPT_API IntParser
{
public:
    IntParser(std::string_view value);
    
    bool isValid() const;
    bool isPositive() const;

    struct NumberSystem
    {
        unsigned int base;
        int shift;
    };
    const NumberSystem& getNumberSystem() const;
   
    std::optional<unsigned int> getNextDigit();
    bool hasFinished() const;
	
private:
    bool isDelimiter();

    std::string_view parsedValue;
    bool valid{false};
    const NumberSystem* numberSystem{nullptr};
 	bool sign{true};
	unsigned int currPosition{0};
    std::optional<char> currDelimiter;
};

}
