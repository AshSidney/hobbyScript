#pragma once

#include <array>
#include <tuple>
#include <algorithm>
#include <bitset>
#include <type_traits>
#include <string_view>
#include <compare>

namespace CppScript
{

struct Id
{
    std::string_view objectId;
    std::string_view moduleId;

    bool operator==(const Id&) const = default;
};


template <typename E, E value>
struct EnumItem
{
    static constexpr E itemValue{ value };
    std::string_view itemName;
};

template <typename E>
constexpr auto enumItems()
{
	return std::make_tuple();
}

template <typename E>
constexpr bool enumDefined{ std::tuple_size_v<decltype(enumItems<E>())> > 0 };

template<>
constexpr bool enumDefined<bool>{ true };

template<>
constexpr bool enumDefined<std::strong_ordering>{ true };
template<>
constexpr bool enumDefined<std::weak_ordering>{ true };
template<>
constexpr bool enumDefined<std::partial_ordering>{ true };


template <typename E, typename V = void>
struct EnumTraits
{
private:
    static_assert(enumDefined<E>);
    static_assert(std::is_same_v<std::tuple_element_t<0, decltype(enumItems<E>())>, Id>);

    template <typename T>
    static constexpr auto makeSequence(T items)
    {
        return []<std::size_t ... I>(std::index_sequence<I...>)
        {
            return std::index_sequence<I + 1 ...>{};
        }(std::make_index_sequence<std::tuple_size_v<T> - 1>{});
    }

    template <typename T, std::size_t ... I>
    static constexpr E minIndexed(T items, std::index_sequence<I...>)
    {
        return static_cast<E>(std::min({ static_cast<int>(std::tuple_element_t<I, T>::itemValue) ... }));
    }

    template <typename T>
    static constexpr E min(T items)
    {
        return minIndexed(items, makeSequence(items));
    }

    template <typename T, std::size_t ... I>
    static constexpr E maxIndexed(T items, std::index_sequence<I...>)
    {
        return static_cast<E>(std::max({ static_cast<int>(std::tuple_element_t<I, T>::itemValue) ... }));
    }

    template <typename T>
    static constexpr E max(T items)
    {
        return maxIndexed(items, makeSequence(items));
    }
   
    static constexpr E minValue{ min(enumItems<E>()) };
    static constexpr E maxValue{ max(enumItems<E>()) };

public:
    static constexpr std::size_t size{ static_cast<int>(maxValue) - static_cast<int>(minValue) + 1 };

    static constexpr Id id{ std::get<0>(enumItems<E>()) };
    
    using Names = std::array<std::string_view, size>;

private:
    template <typename T, std::size_t ... I>
    static Names namesIndexed(const T& items, std::index_sequence<I...>)
    {
        Names names;
        auto setName = [&names](const auto& item){ names[index(item.itemValue)] = item.itemName; };
        (setName(std::get<I>(items)), ... );
        return names;
    }

    static Names names()
    {
        auto items = enumItems<E>();
        return namesIndexed(items, makeSequence(items));
    }

public:
    static Names itemNames;
 
    static constexpr std::size_t index(const E value)
    {
        return static_cast<int>(value) - static_cast<int>(minValue);
    }

    static constexpr E value(const std::size_t index)
    {
        return static_cast<E>(index + static_cast<int>(minValue));
    }
};

 template <typename E, typename V>
 EnumTraits<E, V>::Names EnumTraits<E, V>::itemNames{ EnumTraits<E, V>::names() };


template <>
struct EnumTraits<bool, void>
{
    static constexpr bool minValue{ false };
    static constexpr bool maxValue{ true };
    static constexpr std::size_t size{ 2 };

    static constexpr Id id{ "bool" };
    
    using Names = std::array<std::string_view, size>;
    
    static Names names()
    {
        return { "False", "True" };
    }

    static Names itemNames;

    static constexpr std::size_t index(const bool value)
    {
        return value ? 1 : 0;
    }

    static constexpr bool value(const std::size_t index)
    {
        return index != 0;
    }
};


template <typename T>
constexpr bool isOrdering { std::is_same_v<T, std::strong_ordering>
    || std::is_same_v<T, std::weak_ordering> || std::is_same_v<T, std::partial_ordering> };

template <typename T>
struct EnumTraits<T, typename std::enable_if_t<isOrdering<T>>>
{
    static constexpr T minValue{ T::less };
    static constexpr T maxValue{ T::greater };
    static constexpr std::size_t size{ 3 };

    static constexpr Id id{ "ordering" };
    
    using Names = std::array<std::string_view, size>;
    
    static Names names()
    {
        return { "Less", "Equal", "Greater" };
    }

    static Names itemNames;

    static constexpr std::size_t index(const T value)
    {
        return value == 0 ? 1 : value < 0 ? 0 : 2;
    }

    static constexpr bool value(const std::size_t index)
    {
    	std::array<T, size> orderValues{ T::less, T::equal, T::greater };
        return orderValues[index];
    }
};

template <typename T>
EnumTraits<T, typename std::enable_if_t<isOrdering<T>>>::Names EnumTraits<T, typename std::enable_if_t<isOrdering<T>>>::itemNames{ EnumTraits<T>::names() };


template <typename E>
class EnumFlag
{
public:
    constexpr EnumFlag<E>(std::initializer_list<E> vals)
    {
        for (const E val : vals)
            flags.set(EnumTraits<E>::index(val));
    }

    template <typename E>
    friend constexpr bool operator==(const EnumFlag<E>& left, const EnumFlag<E>& right);

    constexpr bool contains(const EnumFlag<E>& other) const
    {
        return (flags & other.flags) == other.flags;
    }

    constexpr EnumFlag<E>& operator|=(const EnumFlag<E>& other)
    {
        flags |= other.flags;
        return *this;
    }

    constexpr EnumFlag<E>& operator&=(const EnumFlag<E>& other)
    {
        flags &= other.flags;
        return *this;
    }

private:
    static constexpr size_t bitCount { EnumTraits<E>::size };
    std::bitset<bitCount> flags {};
};

template <typename E>
constexpr bool operator==(const EnumFlag<E>& left, const EnumFlag<E>& right)
{
    return left.flags == right.flags;
}

template <typename E>
constexpr EnumFlag<E> operator|(const EnumFlag<E>& left, const EnumFlag<E>& right)
{
    EnumFlag<E> result{left};
    result |= right;
    return result;
}

template <typename E>
constexpr EnumFlag<E> operator&(const EnumFlag<E>& left, const EnumFlag<E>& right)
{
    EnumFlag<E> result{left};
    result &= right;
    return result;
}

}
