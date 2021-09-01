#pragma once

#include <memory>
#include <string>
#include<unordered_map>
#include <sstream>
#include <optional>


namespace CppScript
{
	class TypeIdBase;

	using IntValue = long long;
	using FloatValue = long double;
	using BoolValue = const bool;


	class TypeBase
	{
	public:
		using Ref = std::shared_ptr<TypeBase>;

		virtual ~TypeBase() noexcept = default;

		virtual const TypeIdBase& getId() const = 0;

		template<typename T> std::enable_if_t<std::is_class_v<T>, T&> as();
		template<typename T> std::enable_if_t<std::is_class_v<T>, const T&> as() const;

		template<typename T> std::enable_if_t<std::is_same_v<T, IntValue> || std::is_same_v<T, FloatValue>, T&> as();
		template<typename T> std::enable_if_t<std::is_same_v<T, IntValue> || std::is_same_v<T, FloatValue>, T> as() const;
		template<typename T> std::enable_if_t<std::numeric_limits<T>::is_integer && !std::is_same_v<T, IntValue> && !std::is_same_v<T, BoolValue>, T> as() const;
		template<typename T> std::enable_if_t<std::is_floating_point_v<T> && !std::is_same_v<T, FloatValue>, T> as() const;
		template<typename T> std::enable_if_t<std::is_same_v<T, BoolValue>, T> as() const;

		virtual TypeBase::Ref clone() const;
		virtual void operator+=(const TypeBase& obj);

		virtual bool operator==(const TypeBase& obj) const;
		virtual bool operator<(const TypeBase& obj) const;
	};


	class TypeIdBase
	{
	public:
		TypeIdBase(const std::string_view name) noexcept;

		bool operator==(const TypeIdBase& other) const
		{
			return this == &other;
		}

		const std::string_view& getName() const noexcept
		{
			return typeName;
		}

		static const TypeIdBase* find(const std::string_view& name);

	protected:
		std::string_view typeName;

		static std::unique_ptr<std::unordered_map<std::string_view, const TypeIdBase*>> types;
	};


	class InvalidTypeCast : public std::exception
	{
	public:
		InvalidTypeCast(const std::string_view& fromType, const std::string_view& toType) noexcept;

		virtual const char* what() const noexcept override;

	private:
		std::string_view fromTypeName;
		std::string_view toTypeName;
		std::string message;
	};


	template <typename T> class Type;


	template <typename T> class TypeId : public TypeIdBase
	{
	public:
		TypeId(const char* name) noexcept : TypeIdBase(name)
		{}

		const T& get(const TypeBase& obj) const
		{
			if (*this == obj.getId())
				return static_cast<const Type<T>&>(obj).get();
			throw InvalidTypeCast{ obj.getId().getName(), getName() };
		}

		T& get(TypeBase& obj) const
		{
			if (*this == obj.getId())
				return static_cast<Type<T>&>(obj).get();
			throw InvalidTypeCast{ obj.getId().getName(), getName() };
		}
	};


	template <typename T> class TypeOperations : public TypeBase
	{};


	template <typename T> class Type : public TypeOperations<T>
	{
	public:
		Type() noexcept = default;
		Type(T val) noexcept : value(std::move(val))
		{}
		template <typename ...Args> Type(Args... args) : value(args...)
		{}

		using ValueType = T;

		const T& get() const
		{
			return value;
		}

		T& get()
		{
			return value;
		}

		virtual const TypeIdBase& getId() const override
		{
			return typeId;
		}

		static constexpr TypeId<T>& id()
		{
			return typeId;
		}

		using Ref = std::shared_ptr<Type<T>>;

		template<typename ...Args> static Ref create(Args... args)
		{
			return std::make_shared<Type<T>>(args...);
		}

	private:
		T value;

		static TypeId<T> typeId;
	};


	template<typename T> std::enable_if_t<std::is_class_v<T>, T&> TypeBase::as()
	{
		return Type<T>::id().get(*this);
	}

	template<typename T> std::enable_if_t<std::is_class_v<T>, const T&> TypeBase::as() const
	{
		return Type<T>::id().get(*this);
	}
}