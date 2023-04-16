#pragma once

#include <CppScript/IntValue.h>
#include "IntUtils.h"
#include <cmath>

namespace
{

using StdIntType = CppScript::IntValue::StdIntType;
using SegType = CppScript::IntValue::SegType;

const unsigned int byteBitsCount = 8;
const unsigned int segBitsCount = byteBitsCount * sizeof(SegType);
const unsigned int lowBitsCount = segBitsCount - 1;
const SegType topBitMask = 1ull << lowBitsCount;
const SegType lowBitsMask = ~topBitMask;

std::tuple<SegType, SegType> add(const SegType leftOp, const SegType rightOp, const SegType carry)
{
	const SegType topBit = (leftOp ^ rightOp) & topBitMask;
	const SegType lowSum = (leftOp & lowBitsMask) + (rightOp & lowBitsMask) + carry;
	const SegType newCarry = ((leftOp & rightOp) | (lowSum & topBit)) >> lowBitsCount;
	return { lowSum ^ topBit, newCarry };
}

std::tuple<SegType, SegType> subtract(const SegType leftOp, const SegType rightOp, const SegType carry)
{
	const SegType topBit = ~(leftOp ^ rightOp) & topBitMask;
	const SegType lowDiff = (leftOp | topBitMask) - (rightOp & lowBitsMask) - carry;
	const SegType newCarry = ((~leftOp & rightOp) | (~lowDiff & topBit)) >> lowBitsCount;
	return { lowDiff ^ topBit, newCarry };
}

const unsigned int halfBitsCount = segBitsCount / 2;
const SegType halfSegValue = 1ull << halfBitsCount;
const SegType bottomHalfMask = halfSegValue - 1;
const SegType topHalfMask = ~bottomHalfMask;

std::tuple<SegType, SegType> multiply(const SegType leftOp, const SegType rightOp)
{
	const SegType leftOpBottom = leftOp & bottomHalfMask;
	const SegType leftOpTop = (leftOp & topHalfMask) >> halfBitsCount;
	const SegType rightOpBottom = rightOp & bottomHalfMask;
	const SegType rightOpTop = (rightOp & topHalfMask) >> halfBitsCount;
	const SegType bottomResult = leftOpBottom * rightOpBottom;
	const auto [midResult, midResultCarry] = add(leftOpBottom * rightOpTop, leftOpTop * rightOpBottom, 0);
	const SegType topResult = leftOpTop * rightOpTop;
	const auto [bottomMidSum, bottomMidCarry] = add(bottomResult, (midResult & bottomHalfMask) << halfBitsCount, 0);
	const auto [topMidSum, topMidCarry] = add(topResult, (midResult & topHalfMask) >> halfBitsCount,
		(midResultCarry << halfBitsCount) + bottomMidCarry);
	_ASSERT(topMidCarry == 0);
	return { bottomMidSum, topMidSum };
}

std::tuple<SegType, SegType> divide(const SegType topLeft, const SegType midLeft, const SegType bottomLeft, const SegType rightOp, const unsigned int shift)
{
	const auto invShift = segBitsCount - shift;
	SegType remainder = topLeft << shift | midLeft >> invShift;
	const SegType topResult = remainder / rightOp;
	const SegType bottomRem = midLeft << shift | bottomLeft >> invShift;
	remainder = (remainder - topResult * rightOp) << halfBitsCount | bottomRem >> halfBitsCount;
	const SegType midResult = (remainder / rightOp) << halfBitsCount;
	remainder = (remainder - midResult * rightOp) << halfBitsCount | bottomRem & lowBitsMask;
	const SegType lowResult = remainder / rightOp;
	const auto [stdResult, carry] = add(midResult, lowResult, 0);
	return { stdResult, topResult + carry};
}

bool areUpperBitsZero(const SegType value, const unsigned int bitPosition)
{
	return (value & ~((1ull << bitPosition) - 1)) == 0;
}

unsigned int getTopBit(const SegType value)
{
	unsigned int currBit = halfBitsCount;
	for (unsigned int bitStep = currBit >> 1; bitStep > 0; bitStep >>= 1)
		if (areUpperBitsZero(value, currBit))
			currBit -= bitStep;
		else
			currBit += bitStep;
	return areUpperBitsZero(value, currBit) ? currBit - 1 : currBit;
}


// deprecated
const auto StandardMax = std::numeric_limits<StdIntType>::max() >> 1;
const auto StandardMultMax = static_cast<StdIntType>(std::sqrt(std::numeric_limits<StdIntType>::max()));

const size_t longBitCount = segBitsCount;

}


namespace CppScript
{

DivideByZeroExpection::DivideByZeroExpection() : std::logic_error("Divide by zero")
{}


IntValue::IntValue(StdIntType value)
	: segments{static_cast<SegType>(std::abs(value))}, sign(value >= 0)
{}

IntValue::IntValue(std::string_view value)
{
	IntParser parser{value};
	sign = parser.isPositive();
	auto parseValue = [this, &parser](auto shiftFnc)
	{
		std::optional<unsigned int> digit;
			while(digit = parser.getNextDigit())
			{
				shiftFnc();
				segments.add(NumSegments{*digit}, 0);
			}
	};
	const auto& numSystem = parser.getNumberSystem();
	if (numSystem.shift > 0)
		parseValue([this, shift=numSystem.shift](){ segments.shiftLeft(shift); });
	else
	{
		const NumSegments base{numSystem.base};
		parseValue([this, &base](){ segments.multiply(base); });
	}
	if (!parser.isValid())
		segments.clear();
}

IntValue::IntValue(NumSegments&& sgmnts, const bool sgn)
	: segments(std::move(sgmnts)), sign(sgn || segments.isZero())
{}

IntValue& IntValue::operator+=(const IntValue& otherValue)
{
	addSubtract<true>(otherValue);
	return *this;
}

IntValue& IntValue::operator-=(const IntValue& otherValue)
{
	addSubtract<false>(otherValue);
	return *this;
}

IntValue& IntValue::operator*=(const IntValue& otherValue)
{
	sign = sign == otherValue.sign;
	segments.multiply(otherValue.segments);
	return *this;
}

IntValue& IntValue::operator/=(const IntValue& otherValue)
{
	*this = divide(otherValue);
	return *this;
}

IntValue& IntValue::operator%=(const IntValue& otherValue)
{
	divide(otherValue);
	return *this;
}

std::optional<IntValue::StdIntType> IntValue::getStdInt() const
{
	const auto result = segments.getStdInt();
	if (result)
		return sign ? *result : -*result;
	return {};
}

IntValue IntValue::divide(const IntValue& otherValue)
{
	IntValue result { segments.divide(otherValue.segments), sign == otherValue.sign };
	sign = sign || segments.isZero();
	return result;
}

std::tuple<SegType, SegType> IntValue::add(const SegType leftOp, const SegType rightOp)
{
	return ::add(leftOp, rightOp, 0);
}


template <bool isAdd> void IntValue::addSubtract(const IntValue& otherValue)
{
	if ((sign == otherValue.sign) == isAdd)
		segments.add(otherValue.segments, 0);
	else
	{
		if (segments.less(otherValue.segments))
		{
			const NumSegments tempSegments{ std::move(segments) };
			segments = otherValue.segments;
			sign = otherValue.sign == isAdd;
			segments.subtract(tempSegments, 0);
		}
		else
		{
			segments.subtract(otherValue.segments, 0);
		}
	}
}

IntValue::NumSegments::NumSegments(SegType value) : values{value}
{}

std::optional<IntValue::StdIntType> IntValue::NumSegments::getStdInt() const
{
	if (values.size() == 1)
		return static_cast<StdIntType>(values[0]);
	return {};
}

const std::vector<SegType>& IntValue::NumSegments::getSegments() const
{
	return values;
}

void IntValue::NumSegments::clear()
{
	values.clear();
}

bool IntValue::NumSegments::equal(const NumSegments& other) const
{
	return values == other.values;
}

bool IntValue::NumSegments::less(const NumSegments& other) const
{
	if (values.size() != other.values.size())
		return values.size() < other.values.size();
	auto otherIt = other.values.crbegin();
	for (auto thisIt = values.crbegin(); thisIt != values.crend(); ++thisIt, ++otherIt)
		if (*thisIt != *otherIt)
			return *thisIt < *otherIt;
	return false;
}

bool IntValue::NumSegments::isZero() const
{
	return values.size() == 1 && values.front() == 0;
}

void IntValue::NumSegments::add(const NumSegments& addSegments, const size_t offset)
{
	if (values.size() < addSegments.values.size() + offset)
		values.resize(addSegments.values.size() + offset, 0);
	SegType carry = 0;
	auto segment = values.begin() + offset;
	for (auto addSegment = addSegments.values.begin(); addSegment != addSegments.values.end(); ++segment, ++addSegment)
		std::tie(*segment, carry) = ::add(*segment, *addSegment, carry);
	for (; carry != 0 && segment != values.end(); ++segment)
		std::tie(*segment, carry) = ::add(*segment, 0, carry);
	if (carry != 0)
		values.push_back(carry);
}

void IntValue::NumSegments::subtract(const NumSegments& subSegments, const size_t offset)
{
	_ASSERT(values.size() >= subSegments.values.size() + offset);
	SegType carry = 0;
	auto segment = values.begin() + offset;
	for (auto subSegment = subSegments.values.begin(); subSegment != subSegments.values.end(); ++segment, ++subSegment)
		std::tie(*segment, carry) = ::subtract(*segment, *subSegment, carry);
	for (; carry != 0 && segment != values.end(); ++segment)
		std::tie(*segment, carry) = ::subtract(*segment, 0, carry);
	_ASSERT(carry == 0);
	trim();
}

void IntValue::NumSegments::multiply(const NumSegments& multSegments)
{
	const std::vector<SegType> thisValues{std::move(values)};
	values.reserve(thisValues.size() + multSegments.values.size());
	NumSegments partialResult;
	partialResult.values.reserve(multSegments.values.size() + 1);
	for (unsigned int index = 0; index < thisValues.size(); ++index)
	{
		partialResult.clear();
		SegType multCarry = 0;
		SegType addCarry = 0;
		for (const auto multSegment : multSegments.values)
		{
			auto [segMult, newMultCarry] = ::multiply(thisValues[index], multSegment);
			std::tie(segMult, addCarry) = ::add(segMult, multCarry, addCarry);
			partialResult.values.push_back(segMult);
			multCarry = newMultCarry;
		}
		multCarry += addCarry;
		if (multCarry > 0)
			partialResult.values.push_back(multCarry);
		add(partialResult, index);
	}
}

IntValue::NumSegments IntValue::NumSegments::divide(const NumSegments& divSegments)
{
	if (divSegments.equal(NumSegments{ 0 }))
		throw DivideByZeroExpection{};
	const bool noShift = divSegments.values.back() <= halfSegValue && divSegments.values.size() == 1;
	const unsigned int shift = noShift ? 0
		: (segBitsCount + halfBitsCount - 1 - getTopBit(divSegments.values.back())) % segBitsCount;
	shiftLeft(shift);
	NumSegments shiftedDiv{divSegments};
	shiftedDiv.shiftLeft(shift);
	const size_t divSize = shiftedDiv.values.size();
	const auto topDiv = shiftedDiv.values.back() + (noShift ? 0 : 1);
	NumSegments result{ 0 };
	result.values.reserve(values.size() - divSize + 1);
	NumSegments partialResult;
	partialResult.values.reserve(divSize + 1);
	size_t prevIndex = values.size();
	while (values.size() > divSize)
	{
		const auto index = values.size() - 1;
		const SegType topResult = values[index] / topDiv;
		const SegType midReminder = (values[index] - topDiv * topResult) << halfBitsCount | values[index - 1] >> halfBitsCount;
		const SegType midResult = midReminder / topDiv;
		const SegType bottomReminder = (midReminder - topDiv * midResult) << halfBitsCount | (values[index - 1] & bottomHalfMask);
		const SegType bottomResult = bottomReminder / topDiv;
		partialResult.clear();
		partialResult.values.push_back(bottomResult + (midResult << halfBitsCount));
		if (topResult > 0)
			partialResult.values.push_back(topResult);
		if (prevIndex > index)
			result.values.insert(result.values.begin(), prevIndex - index, 0);
		result.add(partialResult, 0);
		partialResult.multiply(shiftedDiv);
		subtract(partialResult, index - divSize);
		prevIndex = index;
	}
	while (values.size() == divSize && values.back() >= topDiv)
	{
		partialResult.clear();
		partialResult.values.push_back(values.back() / topDiv);
		result.add(partialResult, 0);
		partialResult.multiply(shiftedDiv);
		subtract(partialResult, 0);
	}
	if (!less(shiftedDiv))
	{
		result.add(NumSegments{ 1 }, 0);
		subtract(shiftedDiv, 0);
	}
	shiftRight(shift);
	result.trim();
	return result;
}

void IntValue::NumSegments::shiftLeft(const unsigned int bitCount)
{
	const int offset = bitCount / segBitsCount;
	if (offset > 0)
		values.insert(values.begin(), offset, 0);

	const unsigned int shift = bitCount - offset * longBitCount;
	if (shift > 0)
	{
		const unsigned int nextShift = longBitCount - shift;
		SegType prevSegment = 0;
		for (auto segment = std::next(values.begin(), offset); segment != values.end(); ++segment)
		{
			const auto nextSegment = *segment >> nextShift;
			*segment = (*segment << shift) | prevSegment;
			prevSegment = nextSegment;
		}
		if (prevSegment != 0)
			values.push_back(prevSegment);
	}
}

void IntValue::NumSegments::shiftRight(const unsigned int bitCount)
{
	const unsigned int offset = bitCount / longBitCount;
	if (offset > 0)
	{
		values.erase(values.begin(), std::next(values.begin(), offset));
		if (values.empty())
		{
			values.push_back(0);
			return;
		}
	}

	const unsigned int shift = bitCount - offset * longBitCount;
	if (shift > 0)
	{
		const int nextShift = longBitCount - shift;
		auto segment = values.begin();
		for (auto nextSegment = std::next(segment); nextSegment != values.end(); ++segment, ++nextSegment)
			*segment = (*segment >> shift) | (*nextSegment << nextShift);
		*segment >>= shift;
		trim();
	}
}

void IntValue::NumSegments::trim()
{
	auto topNonZero = std::find_if(values.crbegin(), values.crend(), [](auto segment) {return segment != 0; });
	if (topNonZero != values.crbegin())
		values.erase(topNonZero == values.crend() ? std::next(values.cbegin()) : topNonZero.base(), values.end());
}


IntValue operator ""_I(const char * value)
{
	return IntValue{ value };
}

bool operator==(const IntValue& left, const IntValue& right)
{
	return left.sign == right.sign && left.segments.equal(right.segments);
}

bool operator<(const IntValue& left, const IntValue& right)
{
	if (left.sign != right.sign)
		return right.sign;
	if (left == right)
		return false;
	if (left.sign)
		return left.segments.less(right.segments);
	return right.segments.less(left.segments);
}

IntValue operator+(const IntValue& left, const IntValue& right)
{
	IntValue result {left};
	result += right;
	return result;
}

IntValue operator-(const IntValue& value)
{
	auto negValue { value };
	negValue.sign = !negValue.sign;
	return negValue;
}

IntValue operator-(const IntValue& left, const IntValue& right)
{
	IntValue result {left};
	result -= right;
	return result;
}

IntValue operator*(const IntValue& left, const IntValue& right)
{
	IntValue result {left};
	result *= right;
	return result;
}

IntValue operator/(const IntValue& left, const IntValue& right)
{
	IntValue temp{left};
	return temp.divide(right);
}

IntValue operator%(const IntValue& left, const IntValue& right)
{
	IntValue result{left};
	result.divide(right);
	return result;
}

std::ostream& operator<<(std::ostream& stream, const IntValue& value)
{
	if (!value.sign)
		stream << "-";

	if ((stream.flags() & std::ios::hex) != 0)
	{
		bool showBase = (stream.flags() & std::ios::showbase) != 0;
		for (auto it = value.segments.getSegments().crbegin(); it != value.segments.getSegments().crend(); ++it)
			stream << *it << std::noshowbase;
		if (showBase)
			stream << std::showbase;
	}
	else
	{
		auto remainingValue = value.segments;
		const IntValue::NumSegments zero{ 0 };
		const IntValue::NumSegments base{ 10 };
		const char zeroChar { '0' };

		std::vector<char> invRepr;
		while (zero.less(remainingValue))
		{
			auto newRemValue {remainingValue.divide(base)};
			invRepr.push_back(static_cast<char>(remainingValue.getSegments().front()) + zeroChar);
			remainingValue = std::move(newRemValue);
		}
		for (auto it = invRepr.crbegin(); it != invRepr.crend(); ++it)
			stream << *it;
	}
	return stream;
}






IntValueX::IntValueX(StandardImplType value)
{
	set(value);
}

IntValueX::IntValueX(const std::string_view& value)
{
	LongValue longValue = 0;
	longValue.sign = value[0] != '-';
	size_t startIndex = value[0] != '+' && value[0] != '-' ? 0 : 1;
	if (value.size() > startIndex + 2 && value[startIndex] == '0' && value[startIndex + 1] == 'x')
	{
		for (auto digit = std::next(value.begin(), startIndex + 2); digit != value.end(); ++digit)
		{
			LongImplType digitVal = 0;
			if (*digit >= '0' && *digit <= '9')
				digitVal = *digit - '0';
			else if (*digit >= 'A' && *digit <= 'F')
				digitVal = *digit - 'A' + 0xa;
			else if (*digit >= 'a' && *digit <= 'f')
				digitVal = *digit - 'a' + 0xa;
			else
				continue;
			longValue.shiftLeft(4);
			longValue.value.front() = longValue.value.front() | digitVal;
		}
	}
	else
	{
		for (auto digit = std::next(value.begin(), startIndex); digit != value.end(); ++digit)
			if (*digit >= '0' && *digit <= '9')
			{
				longValue.multiply(10);
				longValue.addAbs({ static_cast<LongImplType>(*digit - '0') });
			}
	}
	set(longValue);
}

std::optional<IntValueX::StandardImplType> IntValueX::get() const
{
	if (value.index() == 0)
		return std::get<0>(value).value;
	else
	{
		const auto& longValue = std::get<1>(value);
		if (longValue.value.size() == 1
			&& longValue.value.front() <= static_cast<LongImplType>(std::numeric_limits<StandardImplType>::max()))
		{
			const auto stdValue = static_cast<StandardImplType>(longValue.value.front());
			return longValue.sign ? stdValue : -stdValue;
		}
	}
	return {};
}

IntValueX& IntValueX::operator+=(const IntValueX& otherValue)
{
	switch (value.index() | (otherValue.value.index() << 1))
	{
	case 0:
		set(std::get<0>(value).value + std::get<0>(otherValue.value).value);
		break;
	case 1:
		std::get<1>(value).addSubtract(std::get<0>(otherValue.value).value, true);
		break;
	case 2:
		{
			LongValue newValue = { std::get<0>(value).value };
			newValue.addSubtract(std::get<1>(otherValue.value), true);
			value = std::move(newValue);
			break;
		}
	case 3:
		std::get<1>(value).addSubtract(std::get<1>(otherValue.value), true);
		break;
	}
	return *this;
}

IntValueX& IntValueX::operator-=(const IntValueX& otherValue)
{
	switch (value.index() | (otherValue.value.index() << 1))
	{
	case 0:
		set(std::get<0>(value).value - std::get<0>(otherValue.value).value);
		break;
	case 1:
		std::get<1>(value).addSubtract(std::get<0>(otherValue.value).value, false);
		break;
	case 2:
		{
			LongValue newValue = { std::get<0>(value).value };
			newValue.addSubtract(std::get<1>(otherValue.value), false);
			value = std::move(newValue);
			break;
		}
	case 3:
		std::get<1>(value).addSubtract(std::get<1>(otherValue.value), false);
		break;
	}
	return *this;
}


/*IntValueX operator ""_I(unsigned long long value)
{
	if (value <= static_cast<LongImplType>(std::numeric_limits<StandardImplType>::max()))
		return IntValueX{ static_cast<StandardImplType>(value) };
	IntValueX result{ 0 };
	result.set(IntValueX::LongValue{ {value}, true });
	return result;
}*/

IntValueX operator ""_IX(const char * value)
{
	return IntValueX{ value };
}

bool operator==(const IntValueX& left, const IntValueX& right)
{
	if (left.value.index() != right.value.index())
		return false;
	return (left.value.index() == 0) ? std::get<0>(left.value).value == std::get<0>(right.value).value
		: std::get<1>(left.value).equal(std::get<1>(right.value));
}

IntValueX operator+(const IntValueX& left, const IntValueX& right)
{
	auto sum = left;
	sum += right;
	return sum;
}

IntValueX operator-(const IntValueX& left, const IntValueX& right)
{
	auto diff = left;
	diff -= right;
	return diff;
}

IntValueX operator-(const IntValueX& value)
{
	auto negValue = value;
	if (negValue.value.index() == 0)
		std::get<0>(negValue.value).value = -std::get<0>(negValue.value).value;
	else
		std::get<1>(negValue.value).sign = !std::get<1>(negValue.value).sign;
	return negValue;
}


std::ostream& operator<<(std::ostream& stream, const IntValueX& value)
{
	const auto simpleValue = value.get();
	if (simpleValue)
		stream << *simpleValue;
	else
	{
		const auto longValue = std::get<1>(value.value);
		if (!longValue.sign)
			stream << "-";

		if ((stream.flags() & std::ios::hex) != 0)
		{
			bool showBase = (stream.flags() & std::ios::showbase) != 0;
			for (auto it = longValue.value.crbegin(); it != longValue.value.crend(); ++it)
				stream << *it << std::noshowbase;
			if (showBase)
				stream << std::showbase;
		}
		else
		{
			auto remValue = longValue;
			const IntValueX::LongValue zero{ 0 };
			std::vector<char> invRepr;
			while (zero.lessAbs(remValue))
			{
				auto newRemValue = remValue.divide(10);
				invRepr.push_back(static_cast<char>(remValue.value.front()) + '0');
				remValue = std::move(newRemValue);
			}
			for (auto it = invRepr.crbegin(); it != invRepr.crend(); ++it)
				stream << *it;
		}
	}
	return stream;
}

void IntValueX::set(StandardImplType value)
{
	if (value <= StandardMax && value >= -StandardMax)
		this->value = StandardValue{ value };
	else
		this->value = LongValue{ value };
}

void IntValueX::set(LongValue value)
{
	if (value.value.size() == 1 && value.value[0] <= StandardMax)
	{
		const StandardImplType newValue = static_cast<StandardImplType>(value.value[0]);
		this->value = StandardValue{ value.sign ? newValue : -newValue };
	}
	else
		this->value = std::move(value);
}


IntValueX::StandardValue::StandardValue(StandardImplType intValue)
	: value(intValue), safeMultiply(value <= StandardMultMax && value >= -StandardMultMax)
{}


IntValueX::LongValue::LongValue(StandardImplType value)
	: value{static_cast<LongImplType>(std::abs(value))}, sign{value >= 0}
{}

IntValueX::LongValue::LongValue(std::vector<LongImplType> value, bool sign)
	: value(std::move(value)), sign(sign)
{}

bool IntValueX::LongValue::equal(const LongValue& otherValue) const
{
	return sign == otherValue.sign && value == otherValue.value;
}

bool IntValueX::LongValue::lessAbs(const LongValue& otherValue) const
{
	if (value.size() != otherValue.value.size())
		return value.size() < otherValue.value.size();
	auto otherIt = otherValue.value.rbegin();
	for (auto thisIt = value.rbegin(); thisIt != value.rend(); ++thisIt, ++otherIt)
		if (*thisIt != *otherIt)
			return *thisIt < *otherIt;
	return false;
}

void IntValueX::LongValue::addSubtract(const LongValue& rightValue, bool isAdd)
{
	if ((sign == rightValue.sign) == isAdd)
		addAbs(rightValue.value);
	else
	{
		if (lessAbs(rightValue))
		{
			const LongValue tempValue{ std::move(value), true };
			value = rightValue.value;
			sign = rightValue.sign == isAdd;
			subtractAbs(tempValue.value);
		}
		else
		{
			subtractAbs(rightValue.value);
		}
		trim();
	}
}

void IntValueX::LongValue::addAbs(const std::vector<LongImplType>& addValue, size_t offset)
{
	if (value.size() < addValue.size() + offset)
		value.resize(addValue.size() + offset, 0);
	LongImplType carry = 0;
	auto part = value.begin() + offset;
	for (auto addPart = addValue.begin(); addPart != addValue.end(); ++part, ++addPart)
		std::tie(*part, carry) = ::add(*part, *addPart, carry);
	for (; carry != 0 && part != value.end(); ++part)
		std::tie(*part, carry) = ::add(*part, 0, carry);
	if (carry != 0)
		value.push_back(carry);
}

void IntValueX::LongValue::subtractAbs(const std::vector<LongImplType>& subValue, size_t offset)
{
	_ASSERT(value.size() >= subValue.size() + offset);
	LongImplType carry = 0;
	auto part = value.begin() + offset;
	for (auto subPart = subValue.begin(); subPart != subValue.end(); ++part, ++subPart)
		std::tie(*part, carry) = ::subtract(*part, *subPart, carry);
	for (; carry != 0 && part != value.end(); ++part)
		std::tie(*part, carry) = ::subtract(*part, 0, carry);
	_ASSERT(carry == 0);
}

void IntValueX::LongValue::multiply(const LongValue& multValue)
{
	std::vector<LongImplType> resultData;
	resultData.reserve(value.size() + multValue.value.size());
	LongValue result{ std::move(resultData), sign == multValue.sign };
	std::vector<LongImplType> partialResult;
	partialResult.reserve(multValue.value.size() + 1);
	for (size_t startIndex = 0; startIndex < value.size(); ++startIndex)
	{
		partialResult.clear();
		LongImplType multCarry = 0;
		LongImplType addCarry = 0;
		for (auto part : multValue.value)
		{
			auto [partMult, newMultCarry] = ::multiply(part, value[startIndex]);
			std::tie(partMult, addCarry) = ::add(partMult, multCarry, addCarry);
			partialResult.push_back(partMult);
			multCarry = newMultCarry;
		}
		multCarry += addCarry;
		if (multCarry > 0)
			partialResult.push_back(multCarry);
		result.addAbs(partialResult, startIndex);
	}
	*this = std::move(result);
}

IntValueX::LongValue IntValueX::LongValue::divide(const LongValue& divValue)
{
	sign = sign == divValue.sign;
	LongValue result{ {}, sign };
	if (value.size() == 1 && divValue.value.size() == 1)
	{
		result.value.push_back(value.front() / divValue.value.front());
		value.front() -= divValue.value.front() * result.value.front();
	}
	else
	{
		const LongImplType topDiv = divValue.value.back() + (divValue.value.size() == 1 ? 0 : 1);
		value.push_back(0);
		for (size_t topIndex = value.size() - 1; topIndex >= divValue.value.size(); --topIndex)
		{
			const LongImplType topLeft = (value[topIndex] << halfBitsCount) | (value[topIndex - 1] >> halfBitsCount);
			const LongImplType topResult = topLeft / topDiv;
			const LongImplType bottomLeft = (topLeft - topResult * topDiv) << halfBitsCount | (value[topIndex - 1] & bottomHalfMask);
			const LongImplType bottomResult = bottomLeft / topDiv;
			LongValue partResult{ { bottomResult + ((topResult & bottomHalfMask) << halfBitsCount), topResult >> halfBitsCount }, sign };
			LongValue partMult = divValue;
			partMult.multiply(partResult);
			subtractAbs(partMult.value, topIndex - divValue.value.size());
			partResult.addAbs(result.value, 1);
			result = std::move(partResult);
		}
		trim();
		if (divValue.lessAbs(*this))
		{
			result.addAbs({ 1 });
			subtractAbs(divValue.value);
		}
	}
	trim();
	result.trim();
	return result;
}

void IntValueX::LongValue::shiftRight(size_t bitCount)
{
	const size_t offset = bitCount / longBitCount;
	if (offset >= value.size())
	{
		value.resize(1);
		value.front() = 0;
		return;
	}
	if (offset > 0)
		value.erase(value.begin(), std::next(value.begin(), offset));

	const size_t shift = bitCount - offset * longBitCount;
	if (shift == 0)
		return;
	const size_t nextShift = longBitCount - shift;
	auto part = value.begin();
	for (auto nextPart = std::next(part); nextPart != value.end(); ++part, ++nextPart)
		*part = (*part >> shift) | (*nextPart << nextShift);
	*part = *part >> shift;
	trim();
}

void IntValueX::LongValue::shiftLeft(size_t bitCount)
{
	const size_t offset = bitCount / longBitCount;
	if (offset > 0)
		value.insert(value.begin(), offset, 0);

	const size_t shift = bitCount - offset * longBitCount;
	if (shift == 0)
		return;
	const size_t nextShift = longBitCount - shift;
	LongImplType prevPart = 0;
	for (auto part = std::next(value.begin(), offset); part != value.end(); ++part)
	{
		const auto nextPart = *part >> nextShift;
		*part = (*part << shift) | prevPart;
		prevPart = nextPart;
	}
	if (prevPart != 0)
		value.push_back(prevPart);
}

void IntValueX::LongValue::trim()
{
	auto topNonZero = std::find_if(value.crbegin(), value.crend(), [](auto part) {return part != 0; });
	if (topNonZero != value.crbegin())
		value.erase(topNonZero == value.crend() ? std::next(value.cbegin()) : topNonZero.base(), value.end());
}

}