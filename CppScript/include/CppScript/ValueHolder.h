#pragma once

#include <CppScript/Definitions.h>
#include <stdexcept>
#include <optional>

namespace CppScript
{

struct CPPSCRIPT_API TypeId
{};

CPPSCRIPT_API constexpr bool operator==(const TypeId& left, const TypeId& right)
{
    return &left == &right;
}


class CPPSCRIPT_API ValueHolder
{
public:
    virtual ~ValueHolder() noexcept = default;

    virtual const TypeId& getTypeId() const = 0;
    virtual const TypeId& getSpecTypeId() const = 0;
};


class CPPSCRIPT_API ValueNotAvailable : public std::logic_error
{
public:
	ValueNotAvailable(const TypeId& typeId): std::logic_error("Value not available"), typeId(typeId)
    {}

    const TypeId& typeId;
};


template <typename T>
class TypeValueHolder : public ValueHolder
{
public:
    using ValueRef = std::add_lvalue_reference_t<T>;
    using ValuePtr = std::add_pointer_t<T>;

    ValueRef get() const
    {
        return *value;
    }

    void set(ValueRef val)
    {
        value = &val;
    }

    const TypeId& getTypeId() const
    {
        return typeId;
    }

    static TypeId typeId;

protected:
    ValuePtr value{ nullptr };
};

template <typename T> TypeId TypeValueHolder<T>::typeId;


template <typename T>
class SpecTypeValueHolder : public TypeValueHolder<std::remove_cvref_t<T>>
{
public:
    using ValueType = std::remove_cvref_t<T>;

    void set(T val)
    {
        storedValue = std::move(val);
        TypeValueHolder<ValueType>::set(*storedValue);
    }

    const TypeId& getSpecTypeId() const
    {
        return specTypeId;
    }

    static TypeId specTypeId;

private:
    std::optional<ValueType> storedValue;
};

template <typename T> TypeId SpecTypeValueHolder<T>::specTypeId;


template <typename T>
class SpecTypeValueHolder<T&> : public TypeValueHolder<std::remove_cv_t<T>>
{
public:
    using ValueType = std::remove_cv_t<T>;

    void set(T& val)
    {
        TypeValueHolder<ValueType>::set(val);
    }

    const TypeId& getSpecTypeId() const
    {
        return specTypeId;
    }

    static TypeId specTypeId;
};

template <typename T> TypeId SpecTypeValueHolder<T&>::specTypeId;


template <typename T>
class ValueTraits
{
public:
    using SpecTypeHolder = SpecTypeValueHolder<T>;
    using ValueType = SpecTypeHolder::ValueType;
    using ValueRef = SpecTypeHolder::ValueRef;

	static ValueRef get(ValueHolder& holder)
    {
        return static_cast<TypeValueHolder<ValueType>&>(holder).get();
    }

    static void set(ValueHolder& holder, T val)
    {
        static_cast<SpecTypeHolder&>(holder).set(std::forward<T>(val));
    }
};

}