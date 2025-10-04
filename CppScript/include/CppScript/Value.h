#pragma once

#include <CppScript/Definitions.h>
#include <CppScript/EnumTraits.h>
#include <memory>
#include <algorithm>
#include <utility>
#include <optional>
#include <span>
#include <type_traits>
#include <stdexcept>
#include <cassert>

namespace CppScript
{

class TypeId;

class CPPSCRIPT_API ValueBase
{
public:
    virtual ~ValueBase() noexcept = default;

    virtual const TypeId& getTypeId() const = 0;
};


class CPPSCRIPT_API TypeId
{
public:
    ~TypeId() noexcept = default;
    TypeId(const TypeId&) = delete;
    TypeId(TypeId&&) = delete;
    TypeId& operator=(const TypeId&) = delete;
    TypeId& operator=(TypeId&&) = delete;

    virtual ValueBase* create(void* ptr) const = 0;

    void updateName(Id name) const
    {
        typeName = name;
    }

    struct Layout
	{
	    std::size_t size{ 0 };
	    std::size_t alignment{ 0 };

        constexpr Layout& operator+=(const Layout& other)
        {
            size += other.size;
            alignment = std::max(alignment, other.alignment);
            return *this;
        }

        constexpr Layout& operator*=(const std::size_t count)
        {
            size *= count;
            return *this;
        }

        template <typename T>
        static constexpr Layout create()
        {
            return { sizeof(T), alignof(T) };
        }
	};

    mutable Id typeName;
    Layout layout;
    const TypeId* commonTypeId{ nullptr };
    const TypeId* commonPtrId{ nullptr };
    bool isFinalType{ false };
    bool isConst{ true };
    std::span<std::string_view> enumItems;

    bool isReference() const
    {
        return commonTypeId == nullptr;
    }

    bool accepts(const TypeId& param) const
    {
        return commonPtrId == param.commonPtrId && (!isReference() || isConst || !param.isConst);
    }

protected:
    TypeId() = default;
};

CPPSCRIPT_API constexpr inline bool operator==(const TypeId& left, const TypeId& right)
{
    return &left == &right;
}

CPPSCRIPT_API constexpr inline TypeId::Layout operator+(const TypeId::Layout& left, const TypeId::Layout& right)
{
    TypeId::Layout result{ left };
    result += right;
    return result;
}

CPPSCRIPT_API constexpr inline TypeId::Layout operator*(const TypeId::Layout& layout, const std::size_t count)
{
	TypeId::Layout result { layout };
	result *= count;
    return result;
}

CPPSCRIPT_API constexpr inline TypeId::Layout operator*(const std::size_t count, const TypeId::Layout& layout)
{
    return layout * count;
}


template <typename T>
class ValueTypeId;


class CPPSCRIPT_API ValueNotAvailable : public std::logic_error
{
public:
	ValueNotAvailable(const TypeId& typeId): std::logic_error("Value not available"), typeId(typeId)
    {}

    const TypeId& typeId;
};


template <typename T>
class ValueCommonPtr : public ValueBase
{
public:
    using Type = T;

    T* get() const
    {
        return valuePtr;
    }

	void set(T* source)
	{
		valuePtr = source;
	}
	
	const TypeId& getTypeId() const override
	{
		return typeId;
	}
	
	static const ValueTypeId<T*> typeId;

protected:
    T& getRef() const
    {
        if (valuePtr == nullptr)
            throw ValueNotAvailable{ getTypeId() };
        return *valuePtr;
    }

    template <typename V>
    void setPtr(V* source)
    {
        valuePtr = const_cast<T*>(source);
    }

private:
    T* valuePtr{ nullptr };
};

template <typename T>
const ValueTypeId<T*> ValueCommonPtr<T>::typeId;


template <typename T>
class ValueCommon : public ValueCommonPtr<T>
{
public:
    using Type = T;
    using Base = ValueCommonPtr<T>;

    T& get() const
    {
        return Base::getRef();
    }

    void set(T&& source)
    {
        value = std::forward<T>(source);
        Base::set(&*value);
    }
	
	const TypeId& getTypeId() const override
	{
		return typeId;
	}
	
	static const ValueTypeId<T> typeId;

private:
    std::optional<T> value;
};

template <typename T>
const ValueTypeId<T> ValueCommon<T>::typeId{ ValueCommon<T>::Base::typeId, false };


template <typename T>
using TypeCommon = std::remove_cv_t<T>;


template <typename T>
class Value : public ValueCommon<TypeCommon<T>>
{
public:
    using Base = ValueCommon<TypeCommon<T>>;

	const TypeId& getTypeId() const override
	{
		return typeId;
	}
	
	static const ValueTypeId<T> typeId;
};

template <typename T>
const ValueTypeId<T> Value<T>::typeId{ Value<T>::Base::typeId, true };


template <typename T>
class Value<T*> : public ValueCommonPtr<TypeCommon<T>>
{
public:
    using Base = ValueCommonPtr<TypeCommon<T>>;

    void set(T* source)
    {
        Base::setPtr(source);
    }

	const TypeId& getTypeId() const override
	{
		return typeId;
	}
	
	static const ValueTypeId<T*> typeId;
};

template <typename T>
const ValueTypeId<T*> Value<T*>::typeId{ Value<T*>::Base::typeId, true };


template <typename T>
class Value<T&> : public ValueCommonPtr<TypeCommon<T>>
{
public:
    using Base = ValueCommonPtr<TypeCommon<T>>;

    T& get() const
    {
        return Base::getRef();
    }

    void set(T& source)
    {
        Base::setPtr(&source);
    }

	const TypeId& getTypeId() const override
	{
		return typeId;
	}
	
	static const ValueTypeId<T&> typeId;
};

template <typename T>
const ValueTypeId<T&> Value<T&>::typeId{ Value<T&>::Base::typeId, true };


template <typename T>
class Value<T[]> : public ValueCommonPtr<TypeCommon<T>>
{
public:
    using Base = ValueCommonPtr<TypeCommon<T>>;

    void set(T source[])
    {
        Base::setPtr(source);
    }

	const TypeId& getTypeId() const override
	{
		return typeId;
	}
	
	static const ValueTypeId<T[]> typeId;
};

template <typename T>
const ValueTypeId<T[]> Value<T[]>::typeId{ Value<T[]>::Base::typeId, true };


template <typename T, std::size_t N>
class Value<T[N]> : public ValueCommonPtr<TypeCommon<T>>
{
public:
    using Base = ValueCommonPtr<TypeCommon<T>>;

    void set(T source[N])
    {
        Base::setPtr(source);
    }

	const TypeId& getTypeId() const override
	{
		return typeId;
	}
	
	static const ValueTypeId<T[N]> typeId;
};

template <typename T, std::size_t N>
const ValueTypeId<T[N]> Value<T[N]>::typeId{ Value<T[N]>::Base::typeId, true };


template <typename T>
class ValueTypeId : public TypeId
{
public:
    constexpr ValueTypeId() = default;

    using ValueType = Value<T>;
    
    constexpr ValueTypeId(const TypeId& baseId, bool finalType)
    {
        if (baseId.commonPtrId == nullptr)
        {
            commonPtrId = &baseId;
        }
        else
        {
            commonTypeId = &baseId;
            commonPtrId = baseId.commonPtrId;
        }
        isConst = std::is_const_v<std::remove_pointer_t<std::remove_reference_t<T>>>;
        isFinalType = finalType;
        if (isFinalType)
        {
            layout = Layout::create<ValueType>();
        }
        if constexpr (enumDefined<T>)
        {
        	typeName = EnumTraits<T>::id;
        	enumItems = EnumTraits<T>::itemNames;
        }
    }

    ValueBase* create(void* ptr) const override
    {
        assert(isFinalType);
        return new(ptr) ValueType;
    }
};

}
