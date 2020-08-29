#include <CppScript/BasicTypes.h>

namespace CppScript
{

template class Type<IntValue>;
TypeId<IntValue> Type<IntValue>::typeId{ "int" };

template class Type<FloatValue>;
TypeId<FloatValue> Type<FloatValue>::typeId{ "float" };

template class Type<BoolValue>;
TypeId<BoolValue> Type<BoolValue>::typeId{ "bool" };

const TypeBase::Ref TypeOperations<BoolValue>::trueValue{ TypeBool::create(true) };
const TypeBase::Ref TypeOperations<BoolValue>::falseValue{ TypeBool::create(false) };


TypeBase::Ref TypeOperations<IntValue>::clone() const
{
	return Type<IntValue>::create(getThis().get());
}

TypeBase::Ref TypeOperations<IntValue>::operator+=(const TypeBase& obj)
{
	getThis().get() += obj.as<IntValue>();
	return shared_from_this();
}

bool TypeOperations<IntValue>::operator==(const TypeBase& obj) const
{
	if (obj.getId() == TypeFloat::id())
		return getThis().get() == obj.as<FloatValue>();
	return getThis().get() == obj.as<IntValue>();
}

bool TypeOperations<IntValue>::operator<(const TypeBase& obj) const
{
	if (obj.getId() == TypeFloat::id())
		return getThis().get() < obj.as<FloatValue>();
	return getThis().get() < obj.as<IntValue>();
}

const Type<IntValue>& TypeOperations<IntValue>::getThis() const
{
	return static_cast<const Type<IntValue>&>(*this);
}

Type<IntValue>& TypeOperations<IntValue>::getThis()
{
	return static_cast<Type<IntValue>&>(*this);
}


TypeBase::Ref TypeOperations<FloatValue>::clone() const
{
	return Type<FloatValue>::create(getThis().get());
}

static FloatValue getFloatOrIntAsFloat(const TypeBase& obj)
{
	if (obj.getId() == Type<IntValue>::id())
		return FloatValue(obj.as<IntValue>());
	return obj.as<FloatValue>();
}

TypeBase::Ref TypeOperations<FloatValue>::operator+=(const TypeBase& obj)
{
	getThis().get() += getFloatOrIntAsFloat(obj);
	return shared_from_this();
}

bool TypeOperations<FloatValue>::operator==(const TypeBase& obj) const
{
	return getThis().get() == getFloatOrIntAsFloat(obj);
}

bool TypeOperations<FloatValue>::operator<(const TypeBase& obj) const
{
	return getThis().get() < getFloatOrIntAsFloat(obj);
}

const Type<FloatValue>& TypeOperations<FloatValue>::getThis() const
{
	return static_cast<const Type<FloatValue>&>(*this);
}

Type<FloatValue>& TypeOperations<FloatValue>::getThis()
{
	return static_cast<Type<FloatValue>&>(*this);
}


TypeBase::Ref TypeOperations<BoolValue>::clone() const
{
	return getThis().get() ? trueValue : falseValue;
}

bool TypeOperations<BoolValue>::operator==(const TypeBase& obj) const
{
	return getThis().get() == obj.as<BoolValue>();
}

bool TypeOperations<BoolValue>::operator<(const TypeBase& obj) const
{
	return getThis().get() < obj.as<BoolValue>();
}

const Type<BoolValue>& TypeOperations<BoolValue>::getThis() const
{
	return static_cast<const Type<BoolValue>&>(*this);
}

}