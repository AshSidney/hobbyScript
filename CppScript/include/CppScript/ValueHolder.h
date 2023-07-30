#pragma once

#include <CppScript/Definitions.h>
#include <optional>
#include <memory>
#include <stdexcept>

namespace CppScript
{

struct CPPSCRIPT_API TypeLayout
{
    size_t size{ 0 };
    size_t alignment{ 0 };

    constexpr TypeLayout& operator+=(const TypeLayout& other)
    {
        size += other.size;
        alignment = std::max(alignment, other.alignment);
        return *this;
    }
};


class ValueHolder;

class CPPSCRIPT_API TypeId
{
public:
    virtual ~TypeId() noexcept = default;

    virtual ValueHolder* construct(void* ptr) const
    {
        return nullptr;
    }

    const TypeId* basicTypeId{ nullptr };
    TypeLayout layout;
};

CPPSCRIPT_API constexpr bool operator==(const TypeId& left, const TypeId& right)
{
    return &left == &right;
}


template <typename T> class SpecTypeValueHolder;

template <typename T>
class ValueTypeId : public TypeId
{
public:
    using Holder = SpecTypeValueHolder<T>;

    constexpr ValueTypeId()
    {
        basicTypeId = &Holder::typeId;
        layout.size = sizeof(Holder);
        layout.alignment = alignof(Holder);
    }

    ValueHolder* construct(void* ptr) const override
    {
        return new(ptr) Holder;
    }
};


class CPPSCRIPT_API ValueHolder
{
public:
    virtual ~ValueHolder() noexcept = default;

    virtual ValueHolder* constructRef(void* ptr) const = 0;

    virtual const TypeId& getTypeId() const = 0;
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

    ValueHolder* constructRef(void* ptr) const override
    {
        using RefHolder = SpecTypeValueHolder<const ValueRef>;
        RefHolder* refHolder = static_cast<RefHolder*>(RefHolder::specTypeId.construct(ptr));
        refHolder->set(get());
        return refHolder;
    }

    const TypeId& getTypeId() const override
    {
        return typeId;
    }

    static TypeId typeId;

protected:
    ValuePtr value{ nullptr };
};

template <typename T>
TypeId TypeValueHolder<T>::typeId;


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

    const TypeId& getTypeId() const override
    {
        return specTypeId;
    }

    static ValueTypeId<T> specTypeId;

private:
    std::optional<ValueType> storedValue;
};

template <typename T>
ValueTypeId<T> SpecTypeValueHolder<T>::specTypeId;


template <typename T>
class SpecTypeValueHolder<T&> : public TypeValueHolder<std::remove_cv_t<T>>
{
public:
    using ValueType = std::remove_cv_t<T>;

    void set(T& val)
    {
        TypeValueHolder<ValueType>::set(const_cast<ValueType&>(val));
    }

    const TypeId& getTypeId() const override
    {
        return specTypeId;
    }

    static ValueTypeId<T&> specTypeId;
};

template <typename T>
ValueTypeId<T&> SpecTypeValueHolder<T&>::specTypeId;


template <typename T>
class SpecTypeValueHolder<std::unique_ptr<T>> : public TypeValueHolder<std::remove_cv_t<T>>
{
public:
    using ValueType = std::remove_cv_t<T>;

    void set(std::unique_ptr<T> val)
    {
        storedValue = std::move(val);
        TypeValueHolder<ValueType>::value = const_cast<ValueType*>(storedValue.get());
    }

    const TypeId& getTypeId() const override
    {
        return specTypeId;
    }

    static ValueTypeId<std::unique_ptr<T>> specTypeId;

private:
    std::unique_ptr<ValueType> storedValue;
};

template <typename T>
ValueTypeId<std::unique_ptr<T>> SpecTypeValueHolder<std::unique_ptr<T>>::specTypeId;


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