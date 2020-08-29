#include <CppScript/Base.h>

namespace CppScript
{

Elements::Elements(std::initializer_list<Ref> elems) noexcept
{
	for (auto elem : elems)
		elements.push_back(std::move(elem));
}
/*void Elements::accept(VisitorOld& visitor) const
{
	for (auto elem : elements)
		elem->accept(visitor);
}
void Elements::accept(VisitorOld& visitor)
{
	for (auto elem : elements)
		elem->accept(visitor);
}*/

}