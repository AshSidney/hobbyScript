#pragma once

#include <bitset>
#include <initializer_list>
#include <type_traits>

namespace CppScript
{

template <typename T>
struct AlwaysFalse : public std::false_type
{};

template <typename E>
struct EnumTraits
{
    static_assert(AlwaysFalse<E>::value, "Enum traits must be specialized for enum");
};

template <typename E>
class EnumFlag
{
public:
    constexpr EnumFlag<E>(std::initializer_list<E> vals)
    {
        for (const E val : vals)
            flags.set(getIndex(val));
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
    constexpr static size_t getIndex(const E val)
    {
        return static_cast<size_t>(val);
    }

    static const size_t bitCount { getIndex(EnumTraits<E>::last) + 1 };
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
