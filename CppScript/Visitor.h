#pragma once

#include <CppScript/TypeInfo.h>
#include <sstream>

namespace CppScript
{

	template <class... Ts> class Visitor;

	template <class T> class Visitor<T>
	{
	public:
		virtual ~Visitor<T>() = default;

		virtual void visit(const T& visitable) = 0;
		virtual void visit(T& visitable) = 0;
	};

	template <class T, class... Ts> class Visitor<T, Ts...> : public Visitor<Ts...>
	{
	public:
		using Visitor<Ts...>::visit;

		virtual void visit(const T& visitable) = 0;
		virtual void visit(T& visitable) = 0;
	};


	template <typename ...P> struct TypePack
	{
		template <template <typename...> typename T> using apply = T<P...>;
	};


	template <class V, class... Ts> class VisitorDefaultImpl;

	template <class V, class T> class VisitorDefaultImpl<V, T> : public V
	{
	public:
		void visit(const T& visitable) override
		{}
		void visit(T& visitable) override
		{}
	};

	template <class V, class T, class... Ts> class VisitorDefaultImpl<V, T, Ts...> : public VisitorDefaultImpl<V, Ts...>
	{
	public:
		using VisitorDefaultImpl<V, Ts...>::visit;

		void visit(const T& visitable) override
		{}
		void visit(T& visitable) override
		{}
	};

	template <class... Ts> using VisitorDefault = VisitorDefaultImpl<Visitor<Ts...>, Ts...>;


	template <class T, class B, class V> class Visitable : public B
	{
	public:
		virtual void accept(V& visitor) const
		{
			visitor.visit(getThis());
		}

		virtual void accept(V& visitor)
		{
			visitor.visit(getThis());
		}

	private:
		const T& getThis() const
		{
			return static_cast<const T&>(*this);
		}

		T& getThis()
		{
			return static_cast<T&>(*this);
		}
	};

	struct VisitableEmptyBase {};

	template<class T, class V> using VisitableBase = Visitable<T, VisitableEmptyBase, V>;


	template <typename T> class UnexpectedVisit : public std::exception
	{
	public:
		UnexpectedVisit<T>() noexcept : className(getTypeName<T>())
		{
			std::ostringstream output;
			output << "Unexpected type " << className << ": Shouldn't be visited.";
			message = output.str();
		}

		virtual const char* what() const noexcept override
		{
			return message.c_str();
		}

	private:
		const std::string className;
		std::string message;
	};


	template <class V, class... Ts> class VisitorFailingImpl;

	template <class V, class T> class VisitorFailingImpl<V, T> : public V
	{
	public:
		void visit(const T& visitable) override
		{
			throw UnexpectedVisit<T>{};
		}
		void visit(T& visitable) override
		{
			throw UnexpectedVisit<T>{};
		}
	};

	template <class V, class T, class... Ts> class VisitorFailingImpl<V, T, Ts...> : public VisitorFailingImpl<V, Ts...>
	{
	public:
		using VisitorFailingImpl<V, Ts...>::visit;

		void visit(const T& visitable) override
		{
			throw UnexpectedVisit<T>{};
		}
		void visit(T& visitable) override
		{
			throw UnexpectedVisit<T>{};
		}
	};

	template <class... Ts> using VisitorFailing = VisitorFailingImpl<Visitor<Ts...>, Ts...>;

}