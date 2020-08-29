#pragma once

#include <CppScript/Base.h>
#include <CppScript/TypeInfo.h>
#include <string>

namespace CppScript
{

	class TypeDescriptorBase
	{
	public:
		TypeDescriptorBase(std::string&& descr) noexcept;
		virtual ~TypeDescriptorBase() = default;

		const std::string& getDescription() const noexcept;

	protected:
		std::string description;
	};


	class TypeWrapperBase : public Visitable<TypeWrapperBase, Element, ElementVisitor>
	{
	public:
		using Ref = std::shared_ptr<TypeWrapperBase>;

		virtual const TypeDescriptorBase& getDescriptor() const = 0;

		//virtual void accept(VisitorOld& visitor) const override;
		//virtual void accept(VisitorOld& visitor) override;
	};


	class InvalidTypeCastOld : public std::exception
	{
	public:
		InvalidTypeCastOld(const std::string& classFrom, const std::string& classTo) noexcept;

		virtual const char* what() const noexcept override;

	private:
		const std::string& fromClassName;
		const std::string& toClassName;
		std::string message;
	};


	template <typename T> class TypeWrapper;


	template <typename T> class TypeDescriptor : public TypeDescriptorBase
	{
	public:
		TypeDescriptor() noexcept : TypeDescriptorBase(getTypeName<T>())
		{}

		const T& get(const TypeWrapperBase& obj) const
		{
			if (this != &obj.getDescriptor())
				throw InvalidTypeCastOld{ obj.getDescriptor().getDescription(), getDescription() };
			return static_cast<const TypeWrapper<T>&>(obj).get();
		}

		T& get(TypeWrapperBase& obj) const
		{
			if (this != &obj.getDescriptor())
				throw InvalidTypeCastOld{ obj.getDescriptor().getDescription(), getDescription() };
			return static_cast<TypeWrapper<T>&>(obj).get();
		}
	};

	template <typename T> class TypeWrapper : public TypeWrapperBase
	{
	public:
		TypeWrapper() noexcept = default;
		TypeWrapper(T val)  noexcept: value(val)
		{}

		const T& get() const
		{
			return value;
		}

		T& get()
		{
			return value;
		}

		static TypeDescriptor<T> typeDescriptor;

		virtual const TypeDescriptorBase& getDescriptor() const override
		{
			return typeDescriptor;
		}

	private:
		T value;
	};

	template <typename T> TypeDescriptor<T> TypeWrapper<T>::typeDescriptor;


	template <typename T> class TypeWrapper<T*> : public TypeWrapperBase
	{
	public:
		TypeWrapper() noexcept = default;
		TypeWrapper(T& val)  noexcept : pointer(&val)
		{}

		const T* get() const
		{
			return pointer;
		}

		T*& get()
		{
			return pointer;
		}

		virtual const TypeDescriptorBase& getDescriptor() const override
		{
			return TypeWrapper<T>::typeDescriptor;
		}

	private:
		T* pointer{ nullptr };
	};


	template <typename T> class TypeValueExtractor : public ElementVisitorFailing
	{
	public:
		void visit(const TypeWrapperBase& wrapper)
		{
			value.get() = &TypeWrapper<T>::typeDescriptor.get(wrapper);
		}

		void visit(TypeWrapperBase& wrapper)
		{
			const TypeWrapperBase& constWrapper{ wrapper };
			visit(constWrapper);
		}

		TypeWrapper<const T*> value;
	};

	template <typename T> const T* getValuePtr(const Element::Ref element)
	{
		TypeValueExtractor<T> valueExtractor;
		element->accept(valueExtractor);
		return valueExtractor.value.get();
	}

	template <typename T> const T& getValue(const Element::Ref element)
	{
		return *getValuePtr<T>(element);
	}


	/*template class TypeWrapper<int>;
	TypeDescriptor<int> TypeWrapper<int>::typeDescriptor;
	template class TypeWrapper<long long>;
	TypeDescriptor<long long> TypeWrapper<long long>::typeDescriptor;
	template class TypeWrapper<float>;
	TypeDescriptor<float> TypeWrapper<float>::typeDescriptor;
	template class TypeWrapper<std::string>;
	TypeDescriptor<std::string> TypeWrapper<std::string>::typeDescriptor;*/
}