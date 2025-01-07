#pragma once

#include <CppScript/Definitions.h>
#include <optional>
#include <memory>
#include <stdexcept>
#include <cassert>

namespace CppScript
{

struct CPPSCRIPT_API TypeLayout
{
    size_t size{ 0 };
    size_t alignment{ 0 };

    template <typename T>
    static constexpr TypeLayout make(const size_t count = 1)
    {
        return { sizeof(T) * count, alignof(T) };
    }

    constexpr TypeLayout& operator+=(const TypeLayout& other)
    {
        size += other.size;
        alignment = std::max(alignment, other.alignment);
        return *this;
    }
};

constexpr TypeLayout operator+(const TypeLayout& left, const TypeLayout& right)
{
    TypeLayout result{left};
    result += right;
    return result;
}


class ValueHolder;

class CPPSCRIPT_API TypeId
{
public:
    virtual ~TypeId() noexcept = default;

    virtual ValueHolder* construct(void* ptr) const
    {
        return nullptr;
    }

    virtual ValueHolder* constructRef(void* ptr, const ValueHolder& source) const
    {
        return nullptr;
    }

    TypeLayout layout;
    const TypeId* basicTypeId{ nullptr };
    const TypeId* refTypeId{ nullptr };
    bool isReference{ false };
};

extern TypeId noTypeId;

CPPSCRIPT_API constexpr bool operator==(const TypeId& left, const TypeId& right)
{
    return &left == &right;
}


class CPPSCRIPT_API ValueHolder
{
public:
    virtual ~ValueHolder() noexcept = default;

    //virtual void set(const ValueHolder& source) = 0;
    //virtual void setRef(const ValueHolder& source) = 0;

    //virtual ValueHolder* constructRef(void* ptr) const = 0;

    virtual const TypeId& getTypeId() const = 0;
    virtual const TypeId& getSpecTypeId() const = 0;
};


template <typename T> class SpecTypeValueHolder;

template <typename T>
class ValueTypeId : public TypeId
{
public:
    using Holder = SpecTypeValueHolder<T>;

    constexpr ValueTypeId()
    {
        layout = TypeLayout::make<Holder>();
        basicTypeId = &Holder::typeId;
        refTypeId = &SpecTypeValueHolder<Holder::ValueRef>::specTypeId;
        isReference = std::is_reference_v<T>;
    }

    ValueHolder* construct(void* ptr) const override
    {
        return new(ptr) Holder;
    }

    ValueHolder* constructRef(void* ptr, const ValueHolder& source) const override
    {
        assert(isReference);
        assert(Holder::typeId == source.getTypeId());
        Holder* refHolder = new(ptr) Holder;
        refHolder->setVal(static_cast<const TypeValueHolder<Holder::ValueType>&>(source).get());
        return refHolder;
    }
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

    void setVal(ValueRef val)
    {
        value = &val;
    }

    /*void setRef(const ValueHolder& source) override
    {
        assert(typeId == source.getTypeId());
        setVal(static_cast<const TypeValueHolder&>(source).get());
    }*/

    /*ValueHolder* constructRef(void* ptr) const override
    {
        using RefHolder = SpecTypeValueHolder<const ValueRef>;
        RefHolder* refHolder = static_cast<RefHolder*>(RefHolder::specTypeId.construct(ptr));
        refHolder->setVal(get());
        return refHolder;
    }*/

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

    void setVal(T val)
    {
        storedValue = std::move(val);
        TypeValueHolder<ValueType>::setVal(*storedValue);
    }

    /*void set(const ValueHolder& source) override
    {
        assert(specTypeId == source.getSpecTypeId());
        setVal(static_cast<const SpecTypeValueHolder<T>&>(source).get());
    }*/

    const TypeId& getSpecTypeId() const override
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

    void setVal(T& val)
    {
        TypeValueHolder<ValueType>::setVal(const_cast<ValueType&>(val));
    }

    /*void set(const ValueHolder& source) override
    {
        assert(specTypeId == source.getSpecTypeId());
        setVal(static_cast<const SpecTypeValueHolder<T&>&>(source).get());
    }*/

    const TypeId& getSpecTypeId() const override
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

    void setVal(std::unique_ptr<T> val)
    {
        storedValue = std::move(val);
        TypeValueHolder<ValueType>::value = const_cast<ValueType*>(storedValue.get());
    }

    /*void set(const ValueHolder& source) override
    {
        assert(specTypeId == source.getSpecTypeId());
        setVal(static_cast<const SpecTypeValueHolder<std::unique_ptr<T>>&>(source).get());
    }*/

    const TypeId& getSpecTypeId() const override
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
        static_cast<SpecTypeHolder&>(holder).setVal(std::forward<T>(val));
    }
};

}