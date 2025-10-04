#pragma once

#include <CppScript/Definitions.h>
#include <CppScript/Execution.h>
#include <CppScript/EnumTraits.h>

#include <string>
#include <optional>
#include <vector>
#include <variant>
#include <compare>
#include <ostream>

namespace CppScript
{

class CPPSCRIPT_API DivideByZeroException : public std::logic_error
{
public:
	DivideByZeroException();
};

enum class ComparisonOld
{
	Less = -1,
	Equal = 0,
	Greater = 1
};

template <ComparisonOld V>
using CompItem = EnumItem<ComparisonOld, V>;

template<>
constexpr auto enumItems<ComparisonOld>()
{
	return std::make_tuple(Id{"Comparison"},
		CompItem<ComparisonOld::Less>{"Less"},
		CompItem<ComparisonOld::Equal>{"Equal"},
		CompItem<ComparisonOld::Greater>{"Greater"});
}

constexpr std::strong_ordering invert(const std::strong_ordering comp)
{
	return comp == 0 ? comp : comp > 0 ? std::strong_ordering::less : std::strong_ordering::greater;
}


class CPPSCRIPT_API IntValue
{
public:
	using StdIntType = long long;
	using SegType = unsigned long long;

	IntValue(StdIntType value);
	IntValue(std::string_view value);

	IntValue(const IntValue& otherValue) = default;
	IntValue(IntValue&& otherValue) = default;
	
	IntValue& operator=(const IntValue& otherValue) = default;
	IntValue& operator=(IntValue&& otherValue) = default;
	
	IntValue& operator+=(const IntValue& otherValue);
	IntValue& operator-=(const IntValue& otherValue);
	IntValue& operator*=(const IntValue& otherValue);
	IntValue& operator/=(const IntValue& otherValue);
	IntValue& operator%=(const IntValue& otherValue);

	friend std::strong_ordering operator<=>(const IntValue& left, const IntValue& right) noexcept;

	friend IntValue operator-(const IntValue& value);

	friend std::ostream& operator<<(std::ostream& stream, const IntValue& value);

	std::optional<StdIntType> getStdInt() const;

	IntValue divide(const IntValue& otherValue);

	static std::tuple<SegType, SegType> add(const SegType leftOp, const SegType rightOp); // for performance testing

private:
	class NumSegments
	{
	public:
		NumSegments() = default;
		explicit NumSegments(SegType value);

		std::optional<StdIntType> getStdInt() const;
		const std::vector<SegType>& getSegments() const;

		void clear();

		std::strong_ordering compare(const NumSegments& other, std::strong_ordering less = std::strong_ordering::less,
			std::strong_ordering greater = std::strong_ordering::greater) const;
		bool isZero() const;

		void add(const NumSegments& addSegments, size_t offset);
		void subtract(const NumSegments& subSegments, size_t offset);
		void multiply(const NumSegments& multSegments);
		NumSegments divide(const NumSegments& divSegments);

		void shiftLeft(unsigned int bitCount);
		void shiftRight(unsigned int bitCount);
		void trim();

	private:
		std::vector<SegType> values;
	};

	IntValue(NumSegments&& sgmnts, bool sgn);

	template <bool isAdd> void addSubtract(const IntValue& otherValue);

	NumSegments segments;
	bool sign {true};
};

IntValue operator ""_I(const char* value);

ComparisonOld compare(const IntValue& left, const IntValue& right) noexcept;
std::strong_ordering operator<=>(const IntValue& left, const IntValue& right) noexcept;
bool operator==(const IntValue& left, const IntValue& right);
IntValue operator+(const IntValue& left, const IntValue& right);
IntValue operator-(const IntValue& value);
IntValue operator-(const IntValue& left, const IntValue& right);
IntValue operator*(const IntValue& left, const IntValue& right);
IntValue operator/(const IntValue& left, const IntValue& right);
IntValue operator%(const IntValue& left, const IntValue& right);

std::ostream& operator<<(std::ostream& stream, const IntValue& value);

}

namespace CppScriptOld
{
	
template <> struct JumpTableTraits<CppScript::ComparisonOld>
{
	static constexpr size_t size = 3;

	static constexpr int index(const CppScript::ComparisonOld val)
	{
		return static_cast<int>(CppScript::EnumTraits<CppScript::ComparisonOld>::index(val));
	}
};

}