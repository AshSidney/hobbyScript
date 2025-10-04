#pragma once

#include <CppScript/Definitions.h>
#include <CppScript/Value.h>
#include <CppScript/EnumTraits.h>
#include <tuple>
#include <span>
#include <vector>
#include <variant>
#include <string>

namespace CppScript
{

struct ValuePlace
{
	enum class Type{ Local, Caller, Module, Void };
	Type placeType{ Type::Void };
	std::size_t index{ 0 };
};

template <ValuePlace::Type V>
using TypeItem = EnumItem<ValuePlace::Type, V>;

template <>
constexpr auto enumItems<ValuePlace::Type>()
{
	return std::make_tuple(Id{"ValuePlaceType"},
		TypeItem<ValuePlace::Type::Local>{"Local"},
		TypeItem<ValuePlace::Type::Caller>{"Caller"},
		TypeItem<ValuePlace::Type::Module>{"Module"},
		TypeItem<ValuePlace::Type::Void>{"Void"});
}

struct OperationBuildContext
{
	std::size_t index{ std::numeric_limits<std::size_t>::max() };
	std::vector<ValuePlace> argumentPlaces;
	std::vector<int> jumps;
};

struct OperationDescription
{
	Id id;
	const TypeId* returnType{ nullptr };
	std::span<const TypeId*> argumentTypes;
	std::size_t jumpCount{ 0 } ;

	bool validate(const OperationBuildContext& context) const;
};


class CPPSCRIPT_API OperationResolver
{
public:
    explicit OperationResolver(std::vector<OperationDescription> descrs);

    struct Resolved
    {
        std::size_t index;
        const OperationDescription& description;
    };

    struct Alternative
    {
        std::string message;
        const OperationDescription& description;
    };

    using Result = std::variant<std::vector<Alternative>, Resolved>;

    Result resolve(const Id& id, const std::vector<const TypeId*>& argTypes) const;
    
    void setModule(std::string_view modId);
    
    void addDescription(OperationDescription descr);

    std::size_t getCount() const;
    bool validate(const OperationBuildContext& context) const;

private:
    struct Description
    {
    	std::size_t index; 
        OperationDescription opDescription;
    };
    
    std::vector<Description> descriptions;
    std::string_view currentModuleId;
};

}