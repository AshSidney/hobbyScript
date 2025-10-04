#pragma once

#include <gmock/gmock.h>

#include <CppScript/Value.h>

namespace CppScriptTest
{

class ValueMock : public CppScript::ValueBase
{
public:
    ValueMock();
    ~ValueMock() noexcept override;

    MOCK_METHOD(const CppScript::TypeId&, getTypeId, (), (const, override));
};

class TypeIdMock : public CppScript::TypeId
{
public:
    TypeIdMock(const CppScript::TypeId::Layout& layout, bool isVal);
    ~TypeIdMock() noexcept;

    MOCK_METHOD(CppScript::ValueBase*, create, (void* ptr), (const, override));
};

template <typename T>
class ValueTypeIdMock : public CppScript::ValueTypeId<T>
{
public:
    constexpr ValueTypeIdMock() : CppScript::ValueTypeId<T>(getBaseTypeId(), true)
    {}

    MOCK_METHOD(CppScript::ValueBase*, create, (void* ptr), (const, override));

    static const CppScript::TypeId& getBaseTypeId()
    {
        auto& valueTypeId = CppScript::Value<T>::typeId;
        return valueTypeId.isReference() ? *valueTypeId.commonPtrId : *valueTypeId.commonTypeId;
    }
 };

template <typename T>
class TestDestruct : public T
{
public:
    ~TestDestruct() noexcept
    {
        if (auto destructList = destructedPointers.lock(); destructList)
            destructList->push_back(this);
    }

    static std::shared_ptr<std::vector<void*>> initDestructList()
    {
        auto destructList = std::make_shared<std::vector<void*>>();
        destructedPointers = destructList;
        return destructList;
    }

private:
    static std::weak_ptr<std::vector<void*>> destructedPointers;
};

template <typename T>
std::weak_ptr<std::vector<void*>> TestDestruct<T>::destructedPointers;


struct TestStruct
{
    int val{ 0 };
    bool flag{ false };
};

using TestStructDestruct = TestDestruct<TestStruct>;

}