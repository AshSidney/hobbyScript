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
	enum class Type{ Local, Caller, Module, Constants };
	Type placeType{ Type::Local };
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
		TypeItem<ValuePlace::Type::Constants>{"Constants"});
}


constexpr std::size_t framesCount{ EnumTraits<ValuePlace::Type>::size };


template <typename T>
constexpr T& getFrame(std::array<T, framesCount>& frames, const ValuePlace::Type placeType)
{
	return frames[EnumTraits<ValuePlace::Type>::index(placeType)];
}


struct OperationLocation
{
    short int line{ 0 };
    short int column{ 0 };
};

struct OperationBuildContext 
{
	std::size_t index{ std::numeric_limits<std::size_t>::max() };
	std::vector<ValuePlace> argumentPlaces;
	std::vector<int> jumps{ 1 };
    OperationLocation location;
};

struct OperationBlockBuildContext
{
	std::vector<const TypeId*> valueTypes;
	std::vector<std::unique_ptr<ValueBase>> constantValues;
	std::vector<OperationBuildContext> operations;
};


struct OperationResolutionData
{
    Id operationId;
    std::optional<ValuePlace> returnPlace;
	std::vector<ValuePlace> argumentPlaces;
	std::vector<int> jumps{ 1 };
    OperationLocation location;
};

struct OperationBlockResolutionData
{
	Id blockId;
    std::vector<std::unique_ptr<ValueBase>> constantValues;
    std::vector<OperationResolutionData> operations;
};

using TypeFrames = std::array<std::vector<const TypeId*>, framesCount>;


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
    struct Error
    {
        Id codeId;
        OperationLocation location;
        std::string message;
        std::vector<const OperationDescription*> alternatives;
    };

    using Result = std::variant<OperationBlockBuildContext, Error>;

    Result resolve(OperationBlockResolutionData&& block, TypeFrames& frames) const;
    
    void addDescriptions(std::initializer_list<OperationDescription> descrs);
    
    const std::vector<OperationDescription>& getDescriptions() const
    {
        return descriptions;
    }

    bool validate(const OperationBuildContext& context) const;

protected:
    struct ResolutionContext
    {
        TypeFrames& frames;
        OperationBlockBuildContext blockContext;
        Error errorData;
        std::vector<const TypeId*> argumentTypes;
    };

    ResolutionContext createContextWithConstants(OperationBlockResolutionData& block, TypeFrames& frames) const;

    bool resolveOperation(OperationResolutionData&& operation, ResolutionContext& context) const;

private:
    std::vector<OperationDescription> descriptions;
};

}