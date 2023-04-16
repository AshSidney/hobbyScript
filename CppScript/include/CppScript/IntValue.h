#pragma once

#include <CppScript/Definitions.h>

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

class CPPSCRIPT_API IntValue
{
public:
	using StdIntType = long long;
	using SegType = unsigned long long;

	IntValue(StdIntType value);
	IntValue(std::string_view value);
	
	IntValue& operator+=(const IntValue& otherValue);
	IntValue& operator-=(const IntValue& otherValue);
	IntValue& operator*=(const IntValue& otherValue);
	IntValue& operator/=(const IntValue& otherValue);
	IntValue& operator%=(const IntValue& otherValue);

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

bool operator==(const IntValue& left, const IntValue& right);
bool operator<(const IntValue& left, const IntValue& right);
IntValue operator+(const IntValue& left, const IntValue& right);
IntValue operator-(const IntValue& value);
IntValue operator-(const IntValue& left, const IntValue& right);
IntValue operator*(const IntValue& left, const IntValue& right);
IntValue operator/(const IntValue& left, const IntValue& right);
IntValue operator%(const IntValue& left, const IntValue& right);

std::ostream& operator<<(std::ostream& stream, const IntValue& value);


class IntValueX
{
public:
	using StandardImplType = long long;
	using LongImplType = unsigned long long;

	IntValueX(StandardImplType value);
	IntValueX(const std::string_view& value);

	std::optional<long long> get() const;

	IntValueX& operator+=(const IntValueX& otherValue);
	IntValueX& operator-=(const IntValueX& otherValue);

	//friend IntValueX operator ""_I(unsigned long long value);
	friend IntValueX operator ""_IX(const char* value);

	friend bool operator==(const IntValueX& left, const IntValueX& right);
	friend IntValueX operator+(const IntValueX& left, const IntValueX& right);
	friend IntValueX operator-(const IntValueX& value);
	friend IntValueX operator-(const IntValueX& left, const IntValueX& right);
	friend std::ostream& operator<<(std::ostream& stream, const IntValueX& value);

	class StandardValue
	{
	public:
		StandardValue(StandardImplType intValue);

		StandardImplType value;
		bool safeMultiply;
	};

	class LongValue
	{
	public:
		LongValue(StandardImplType value);
		LongValue(std::vector<LongImplType> value, bool sign);

		std::vector<LongImplType> value;
		bool sign{ true };

		/*void add(LongImplType addValue);
		void subtract(LongImplType subValue);
		void multiply(LongImplType multValue);*/
		bool equal(const LongValue& otherValue) const;
		bool lessAbs(const LongValue& otherValue) const;

		void addSubtract(const LongValue& addValue, bool isAdd);
		void addAbs(const std::vector<LongImplType>& addValue, size_t offset = 0);
		void subtractAbs(const std::vector<LongImplType>& subValue, size_t offset = 0);
		void multiply(const LongValue& multValue);
		LongValue divide(const LongValue& divValue);

		void shiftRight(size_t bitCount);
		void shiftLeft(size_t bitCount);

		void trim();
	};

private:
	std::variant<StandardValue, LongValue> value{ StandardValue{0} };

	void set(StandardImplType value);
	void set(LongValue value);
};

//IntValueX operator ""_I(unsigned long long value);
IntValueX operator ""_IX(const char* value);

bool operator==(const IntValueX& left, const IntValueX& right);
IntValueX operator+(const IntValueX& left, const IntValueX& right);
IntValueX operator-(const IntValueX& left, const IntValueX& right);
IntValueX operator-(const IntValueX& value);

std::ostream& operator<<(std::ostream& stream, const IntValueX& value);

}

