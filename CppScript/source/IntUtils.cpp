#include "IntUtils.h"
#include <cctype>

namespace
{
    const char zeroOffset = '0';
    const char nineOffset = '9';
    const char hexOffset = 'a' - 10;
    static_assert(zeroOffset < hexOffset);

    const char plus = '+';
    const char minus = '-';

    const char cppDelimiter = '\'';
    const char pythonDelimiter = '_';

    const CppScript::IntParser::NumberSystem decimal{10, 0};
    const CppScript::IntParser::NumberSystem binary{2, 1};
    const CppScript::IntParser::NumberSystem octal{8, 3};
    const CppScript::IntParser::NumberSystem hexadecimal{16, 4};
    const CppScript::IntParser::NumberSystem invalidNumSystem{0, 0};
}

namespace CppScript
{

IntParser::IntParser(std::string_view value)
    : parsedValue(value), valid(!parsedValue.empty()), numberSystem(&invalidNumSystem)
{
    if (!valid)
        return;
    sign = parsedValue[0] != minus;
    currPosition = !sign || parsedValue[0] == plus ? 1 : 0;
    if (parsedValue[currPosition] == zeroOffset && currPosition + 1 < parsedValue.size())
    {
        ++currPosition;
        switch(std::tolower(parsedValue[currPosition]))
        {
        case 'b':
            numberSystem = &binary;
            break;
        case 'o':
            numberSystem = &octal;
            break;
        case 'x':
            numberSystem = &hexadecimal;
            break;
        }
        ++currPosition;
    }
    else if (parsedValue[currPosition] >= zeroOffset && parsedValue[currPosition] <= nineOffset)
        numberSystem = &decimal;
    else
        valid = false;
}
    
bool IntParser::isValid() const
{
    return valid;
}
    
bool IntParser::isPositive() const
{
    return sign;
}

const IntParser::NumberSystem& IntParser::getNumberSystem() const
{
    return *numberSystem;
}
   
std::optional<unsigned int> IntParser::getNextDigit()
{
    if(hasFinished() || !isValid())
        return {};
    if (isDelimiter())
    {
        ++currPosition;
        if (hasFinished())
        {
            valid = false;
            return {};
        }
    }
    const auto currChar = std::tolower(parsedValue[currPosition]);
    unsigned int digit = static_cast<unsigned int>(currChar - zeroOffset);
    if (digit >= numberSystem->base && numberSystem == &hexadecimal)
        digit = static_cast<unsigned int>(currChar - hexOffset);
    if (digit >= numberSystem->base)
    {
        valid = false;
        return {};
    }
    ++currPosition;
    return digit;
}

bool IntParser::hasFinished() const
{
    return currPosition >= parsedValue.size();
}

bool IntParser::isDelimiter()
{
    if (currDelimiter)
        return parsedValue[currPosition] == *currDelimiter;
    if (parsedValue[currPosition] == cppDelimiter || parsedValue[currPosition] == pythonDelimiter)
    {
        currDelimiter = parsedValue[currPosition];
        return true;
    }
    return false;
}

}