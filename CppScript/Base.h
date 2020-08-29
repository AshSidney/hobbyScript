#pragma once

#include <CppScript/Visitor.h>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>


namespace CppScript
{

	using VisitableClasses = TypePack<class Element, class TypeWrapperBase, class Elements, class OperationOld, class ForLoop>;

	using ElementVisitor = VisitableClasses::apply<Visitor>;
	using ElementVisitorDefault = VisitableClasses::apply<VisitorDefault>;
	using ElementVisitorFailing = VisitableClasses::apply<VisitorFailing>;


	class Element : public VisitableBase<Element, ElementVisitor>
	{
	public:
		virtual ~Element() = default;

		//virtual void accept(VisitorOld& visitor) const = 0;
		//virtual void accept(VisitorOld& visitor) = 0;

		template<typename T, typename ...Args> static std::shared_ptr<T> create(Args... args)
		{
			static_assert(std::is_base_of<Element, T>::value, "type is not derived from Element class");
			return std::make_shared<T>(args...);
		}

		using Ref = std::shared_ptr<Element>;
	};


	class Elements : public Visitable<Elements, Element, ElementVisitor>
	{
	public:
		Elements() noexcept = default;
		Elements(std::initializer_list<Ref> elems) noexcept;
		//void accept(VisitorOld& visitor) const override;
		//void accept(VisitorOld& visitor) override;

		std::vector<Ref> elements;
	};


	/*class TypeWrapperBase;
	class ContextReader;
	class ForLoop;
	class Sum;


	class VisitorOld
	{
	public:
		virtual ~VisitorOld() = default;

		virtual void visit(const TypeWrapperBase& wrapper) = 0;
		virtual void visit(TypeWrapperBase& wrapper) = 0;

		virtual void visit(const Elements& frame) = 0;
		virtual void visit(Elements& frame) = 0;

		//virtual void visit(const ContextReader& frame) = 0;
		//virtual void visit(ContextReader& frame) = 0;

		virtual void visit(const ForLoop& control) = 0;
		virtual void visit(ForLoop& control) = 0;

		virtual void visit(const Sum& control) = 0;
		virtual void visit(Sum& control) = 0;
	};


	template<class T> class ElementCast : public VisitorOld
	{
	public:

	};*/

}
