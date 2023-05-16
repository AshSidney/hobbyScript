#pragma once

#include <CppScript/Definitions.h>
#include <CppScript/Execution.h>

#include <string>
#include <optional>
#include <vector>
#include <variant>
#include <ostream>

namespace CppScript
{

class CPPSCRIPT_API DivideByZeroExpection : public std::logic_error
{
public:
	DivideByZeroExpection();
};

enum class Comparison
{
	Less = -1,
	Equal = 0,
	Greater = 1
};

constexpr Comparison invert(const Comparison comp)
{
	return static_cast<Comparison>(-static_cast<int>(comp));
}

template <> struct JumpTableTraits<Comparison>
{
	static constexpr size_t size = 3;

	static constexpr int index(const Comparison val)
	{
		return static_cast<int>(val) - static_cast<int>(Comparison::Less);
	}
};


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

	friend Comparison compare(const IntValue& left, const IntValue& right);
	friend bool operator==(const IntValue& left, const IntValue& right);
	friend bool operator<(const IntValue& left, const IntValue& right);
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
		//explicit NumSegments(std::vector<SegType> segments);

		std::optional<StdIntType> getStdInt() const;
		const std::vector<SegType>& getSegments() const;

		void clear();

		Comparison compare(const NumSegments& other, Comparison less = Comparison::Less, Comparison greater = Comparison::Greater) const;
		bool equal(const NumSegments& other) const;
		bool less(const NumSegments& other) const;
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

Comparison compare(const IntValue& left, const IntValue& right);
bool operator==(const IntValue& left, const IntValue& right);
bool operator<(const IntValue& left, const IntValue& right);
IntValue operator+(const IntValue& left, const IntValue& right);
IntValue operator-(const IntValue& value);
IntValue operator-(const IntValue& left, const IntValue& right);
IntValue operator*(const IntValue& left, const IntValue& right);
IntValue operator/(const IntValue& left, const IntValue& right);
IntValue operator%(const IntValue& left, const IntValue& right);

std::ostream& operator<<(std::ostream& stream, const IntValue& value);

}

