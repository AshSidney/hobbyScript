#pragma once

#include <CppScript/Definitions.h>
#include <memory>
#include <string_view>
#include <unordered_map>

namespace CppScript
{

class TypeIdBase;

class CPPSCRIPT_API ValueHolder
{
public:
    using Ref = std::shared_ptr<ValueHolder>;

    virtual ~ValueHolder() noexcept = default;

    virtual const TypeIdBase& getId() const = 0;

    virtual Ref Clone() const;

};


class CPPSCRIPT_API TypeIdBase
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

private:
    std::string_view typeName;

    static std::unique_ptr<std::unordered_map<std::string_view, const TypeIdBase*>> types;
};

}