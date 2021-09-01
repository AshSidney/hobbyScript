#pragma once

#include <CppScript/Types.h>


namespace CppScript
{

	template <> class TypeOperations<IntValue> : public TypeBase
	{
	public:
		virtual TypeBase::Ref clone() const override;
		virtual void operator+=(const TypeBase& obj) override;
		virtual bool operator==(const TypeBase& obj) const override;
		virtual bool operator<(const TypeBase& obj) const override;

	private:
		const Type<IntValue>& getThis() const;
		Type<IntValue>& getThis();
	};

	template <> class TypeOperations<FloatValue> : public TypeBase
	{
	public:
		virtual TypeBase::Ref clone() const override;
		virtual void operator+=(const TypeBase& obj) override;
		virtual bool operator==(const TypeBase& obj) const override;
		virtual bool operator<(const TypeBase& obj) const override;

	private:
		const Type<FloatValue>& getThis() const;
		Type<FloatValue>& getThis();
	};

	template <> class TypeOperations<BoolValue> : public TypeBase
	{
	public:
		virtual TypeBase::Ref clone() const override;
		virtual bool operator==(const TypeBase& obj) const override;
		virtual bool operator<(const TypeBase& obj) const override;

		static const TypeBase::Ref trueValue;
		static const TypeBase::Ref falseValue;

	private:
		const Type<BoolValue>& getThis() const;
	};


	extern template class Type<IntValue>;
	extern template class Type<FloatValue>;
	extern template class Type<BoolValue>;

	using TypeInt = Type<IntValue>;
	using TypeFloat = Type<FloatValue>;
	using TypeBool = Type<BoolValue>;


	template <typename T, typename S> class ValueOverflow : public std::exception
	{
	public:
		ValueOverflow(const S sourceVal) noexcept : sourceValue(sourceVal)
		{
			std::ostringstream messageStream;
			messageStream << "Value: " << sourceValue << " is outside the range of target type: "
				<< std::numeric_limits<T>::lowest() << " : " << std::numeric_limits<T>::max();
			message = messageStream.str();
		}

		virtual const char* what() const noexcept override
		{
			return message.c_str();
		}

	private:
		const S sourceValue;
		std::string message;
	};


	template<typename T> std::enable_if_t<std::is_same_v<T, IntValue> || std::is_same_v<T, FloatValue>, T&> TypeBase::as()
	{
		return Type<T>::id().get(*this);
	}

	template<typename T> std::enable_if_t<std::is_same_v<T, IntValue> || std::is_same_v<T, FloatValue>, T> TypeBase::as() const
	{
		return Type<T>::id().get(*this);
	}

	template<typename T> std::enable_if_t<std::numeric_limits<T>::is_integer && !std::is_same_v<T, IntValue> && !std::is_same_v<T, BoolValue>, T> TypeBase::as() const
	{
		const auto& value = TypeInt::id().get(*this);
		if (value < std::numeric_limits<T>::lowest() || value > std::numeric_limits<T>::max())
			throw ValueOverflow<T, TypeInt::ValueType>{value};
		return T(value);
	}

	template<typename T> std::enable_if_t<std::is_floating_point_v<T> && !std::is_same_v<T, FloatValue>, T> TypeBase::as() const
	{
		const auto& value = TypeFloat::id().get(*this);
		if (value < std::numeric_limits<T>::lowest() || value > std::numeric_limits<T>::max())
			throw ValueOverflow<T, TypeFloat::ValueType>{value};
		return T(value);
	}

	template<typename T> std::enable_if_t<std::is_same_v<T, BoolValue>, T> TypeBase::as() const
	{
		return TypeBool::id().get(*this);
	}

}