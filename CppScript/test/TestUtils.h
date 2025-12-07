#pragma once

#include <CppScript/Value.h>
#include <CppScript/Operation.h>
#include <CppScript/ValueHolder.h>
#include <CppScript/Execution.h>
#include <iterator>

namespace CppScript
{

constexpr bool operator==(const TypeId::Layout& left, const TypeId::Layout& right)
{
    return left.size == right.size && left.alignment == right.alignment;
}

constexpr bool operator==(const ValuePlace& left, const ValuePlace& right)
{
    return left.placeType == right.placeType && left.index == right.index;
}

constexpr bool operator==(const OperationLocation& left, const OperationLocation& right)
{
    return left.line == right.line && left.column == right.column;
}

template <typename T>
constexpr bool operator==(const std::span<T>& left, const std::span<T>& right)
{
	return left.size() == right.size() && std::equal(right.begin(), right.end(), left.begin());
}

}

namespace CppScriptTest
{

template <typename T>
constexpr std::vector<T> span2Vector(std::span<T> source)
{
	std::vector<T> result;
	std::copy(source.begin(), source.end(), std::back_inserter(result));
	return result;
}

template <std::size_t N, typename T>
constexpr std::array<T, N> span2Array(std::span<T> source)
{
	std::array<T, N> result;
	std::copy(source.begin(), source.end(), result.begin());
	return result;
}

template <typename ... T>
std::vector<std::unique_ptr<CppScript::ValueBase>> makeConstants(T&& ... vals)
{
	std::vector<std::unique_ptr<CppScript::ValueBase>> constants;
	auto pushVal = [&](auto val)
	{
		auto valPtr = std::make_unique<CppScript::Value<const std::decay_t<decltype(val)>>>();
		valPtr->set(std::move(val));
		constants.push_back(std::move(valPtr));
	};
	( pushVal(vals), ... );
	return constants;
}

}


// deprecated
namespace CppScriptOld
{

bool operator==(const TypeLayout& left, const TypeLayout& right);
TypeLayout operator*(const TypeLayout& layout, size_t mult);

bool operator==(const PlaceData& left, const PlaceData& right);


template <typename T>
std::unique_ptr<SpecTypeValueHolder<T>> makeValue(T val)
{
    auto holder = std::make_unique<SpecTypeValueHolder<T>>();
    holder->setVal(val);
    return holder;
}

}
