#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <CppScript/Execution.h>
#include <CppScript/FunctionDef.h>
#include <numeric>
#include <unordered_map>

#include "TestUtils.h"

using namespace CppScript;


class ValueHolderMock : public ValueHolder
{
public:
    ValueHolderMock(const TypeId& id, const ValueHolder* ref = nullptr) : typeId(id), refValue(ref)
    {}

    ~ValueHolderMock()
    {
        if (destructList != nullptr)
            destructList->push_back(this);
    }

   const TypeId& getTypeId() const override
    {
        return typeId;
    }
    const TypeId& getSpecTypeId() const override
    {
        return typeId;
    }

    const TypeId& typeId;
    const ValueHolder* refValue { nullptr };

    static std::vector<const ValueHolder*>* destructList;
};

std::vector<const ValueHolder*>* ValueHolderMock::destructList{ nullptr };


class TypeIdMock : public TypeId
{
public:
    TypeIdMock(const TypeLayout& typeLayout, const bool isRef, const TypeIdMock* refType)
    {
        layout = typeLayout;
        refTypeId = (refType != nullptr || !isRef) ? refType : this;
        isReference = isRef;
    }

    MOCK_METHOD(ValueHolder*, construct, (void* ptr), (const, override));
    MOCK_METHOD(ValueHolder*, constructRef, (void* ptr, const ValueHolder& source), (const, override));

    ValueHolderMock* constructMock(void* ptr, const ValueHolder* source = nullptr)
    {
        return new(ptr) ValueHolderMock(*this, source);
    }

    static std::unique_ptr<TypeIdMock> makeWithCalls(const TypeLayout& layout, const bool isRef, const TypeIdMock* refType)
    {
        auto typeId = std::make_unique<TypeIdMock>(layout, isRef, refType);
        TypeIdMock& typeIdRef{ *typeId };
        EXPECT_CALL(typeIdRef, construct(testing::_))
            .WillRepeatedly([&typeIdRef](void* ptr)
                { return typeIdRef.constructMock(ptr); });
        if (isRef)
        {
            EXPECT_CALL(typeIdRef, constructRef(testing::_, testing::_))
                .WillRepeatedly([&typeIdRef](void* ptr, const ValueHolder& source)
                    { return typeIdRef.constructMock(ptr, &source); });
        }
        return typeId;
    }
};


class MemoryAllocatorMock : public MemoryAllocator
{
public:
    MemoryAllocatorMock(const std::vector<TypeLayout>& layouts)
    {
        std::transform(layouts.begin(), layouts.end(), std::back_inserter(buffers),
            [](const auto& layout)
            {
                return BufferData{std::vector<std::byte>(layout.size), layout };
            });
        nextBuffer = buffers.begin();
        currentAllocator = this;
    }

    ~MemoryAllocatorMock()
    {
        EXPECT_TRUE(allocBuffers.empty());
        for (auto& buff : buffers)
            EXPECT_TRUE(buff.isFree);
        if (this == currentAllocator)
            currentAllocator = nullptr;
    }

	static std::byte* allocate(const TypeLayout& layout)
    {
        return currentAllocator->allocBuff(layout);
    }

	static void free(std::byte* memPtr)
    {
        currentAllocator->freeBuff(memPtr);
    }

	std::byte* allocBuff(const TypeLayout& layout)
    {
        if (layout.size == 0)
            return nullptr;
        currBuffer = nextBuffer++;
        EXPECT_NE(currBuffer, buffers.end());
        EXPECT_EQ(currBuffer->layout, layout);
        auto* memPtr = currBuffer->buffer.data();
        allocBuffers[memPtr] = &*currBuffer;
        currBuffer->isFree = false;
        return memPtr;
    }

	void freeBuff(std::byte* memPtr)
    {
        auto* memBuff = allocBuffers[memPtr];
        EXPECT_FALSE(memBuff->isFree);
        memBuff->isFree = true;
        allocBuffers.erase(memPtr);
    }

    std::byte* cPtr(const size_t offset) const
    {
        return currBuffer->buffer.data() + offset;
    }

    std::byte* nPtr(const size_t offset) const
    {
        return nextBuffer->buffer.data() + offset;
    }

    struct BufferData
    {
        std::vector<std::byte> buffer;
        TypeLayout layout;
        bool isFree{ true };
    };
    
    std::vector<BufferData> buffers;
    std::vector<BufferData>::iterator currBuffer;
    std::vector<BufferData>::iterator nextBuffer;
    std::unordered_map<const std::byte*, BufferData*> allocBuffers;

    static MemoryAllocatorMock* currentAllocator;
};

MemoryAllocatorMock* MemoryAllocatorMock::currentAllocator{ nullptr };


TEST(ExecutionTest, MemoryAllocator_IsPowerTwo)
{
    EXPECT_TRUE(MemoryAllocator::isPowerTwo(0));
    EXPECT_TRUE(MemoryAllocator::isPowerTwo(1));
    EXPECT_TRUE(MemoryAllocator::isPowerTwo(2));
    EXPECT_FALSE(MemoryAllocator::isPowerTwo(3));
    EXPECT_TRUE(MemoryAllocator::isPowerTwo(4));
    EXPECT_FALSE(MemoryAllocator::isPowerTwo(5));
    EXPECT_FALSE(MemoryAllocator::isPowerTwo(6));
    EXPECT_FALSE(MemoryAllocator::isPowerTwo(7));
    EXPECT_TRUE(MemoryAllocator::isPowerTwo(8));
    EXPECT_TRUE(MemoryAllocator::isPowerTwo(0x10));
    EXPECT_FALSE(MemoryAllocator::isPowerTwo(0x14));
    EXPECT_TRUE(MemoryAllocator::isPowerTwo(0x20));
    EXPECT_FALSE(MemoryAllocator::isPowerTwo(0x48));
    EXPECT_TRUE(MemoryAllocator::isPowerTwo(0x80));
}

TEST(ExecutionTest, MemoryAllocator_AlignDown)
{
    EXPECT_EQ(MemoryAllocator::alignDown(100, 8), 96);
    EXPECT_EQ(MemoryAllocator::alignDown(120, 16), 112);
    EXPECT_EQ(MemoryAllocator::alignDown(231, 32), 224);
    EXPECT_EQ(MemoryAllocator::alignDown(160, 8), 160);
}

TEST(ExecutionTest, MemoryAllocator_AlignUp)
{
    EXPECT_EQ(MemoryAllocator::alignUp(100, 8), 104);
    EXPECT_EQ(MemoryAllocator::alignUp(120, 16), 128);
    EXPECT_EQ(MemoryAllocator::alignUp(231, 32), 256);
    EXPECT_EQ(MemoryAllocator::alignUp(160, 8), 160);
}

TEST(ExecutionTest, MemoryAllocator_IsAligned)
{
    EXPECT_FALSE(MemoryAllocator::isAligned(180, 8));
    EXPECT_FALSE(MemoryAllocator::isAligned(428, 16));
    EXPECT_FALSE(MemoryAllocator::isAligned(231, 32));
    EXPECT_TRUE(MemoryAllocator::isAligned(184, 8));
}

TEST(ExecutionTest, MemoryAllocator_AlignUpLayout)
{
    EXPECT_EQ(MemoryAllocator::alignUp({100, 8}), TypeLayout(104, 8));
    EXPECT_EQ(MemoryAllocator::alignUp({130, 16}), TypeLayout(144, 16));
    EXPECT_EQ(MemoryAllocator::alignUp({231, 32}), TypeLayout(256, 32));
    EXPECT_EQ(MemoryAllocator::alignUp({160, 8}), TypeLayout(160, 8));
}


TEST(ExecutionTest, DataTypePlace_MakeValue)
{
    TypeIdMock typeIdMock{ {}, false, nullptr };
    std::array<std::byte, 1024> buffer;
    const DataTypePlace testData{ typeIdMock, 16, nullptr };
    EXPECT_CALL(typeIdMock, construct(buffer.data() + 16))
        .WillOnce([&typeIdMock](void* ptr){ return typeIdMock.constructMock(ptr); });
    const auto value { testData.makeValue(buffer.data()) };
    EXPECT_EQ(static_cast<const void*>(value.holder), buffer.data() + 16);
    EXPECT_TRUE(value.mustDestruct);
}

TEST(ExecutionTest, DataTypePlace_MakeValueRef)
{
    TypeIdMock typeIdMock{ {}, false, nullptr };
    ValueHolderMock valueSource{ typeIdMock };
    TypeIdMock refTypeIdMock{ {}, true, &typeIdMock };
    std::array<std::byte, 1024> buffer;
    const DataTypePlace testData{refTypeIdMock, 32, &valueSource};
    EXPECT_CALL(refTypeIdMock, constructRef(buffer.data() + 32, testing::Ref(valueSource)))
        .WillOnce([&refTypeIdMock](void* ptr, const ValueHolder& source){ return refTypeIdMock.constructMock(ptr, &source); });
    const auto value { testData.makeValue(buffer.data()) };
    EXPECT_EQ(static_cast<const void*>(value.holder), buffer.data() + 32);
    EXPECT_EQ(static_cast<const ValueHolderMock*>(value.holder)->refValue, &valueSource);
    EXPECT_FALSE(value.mustDestruct);
}

TEST(ExecutionTest, DataTypePlace_MakeValueArg)
{
    TypeIdMock typeIdMock{ {}, false, nullptr };
    ValueHolderMock valueArg{ typeIdMock };
    TypeIdMock argTypeIdMock{{}, true, &typeIdMock};
    std::array<std::byte, 1024> buffer;
    const DataTypePlace testData{argTypeIdMock, 48, nullptr};
    testData.setArgument(valueArg);
    EXPECT_CALL(argTypeIdMock, constructRef(buffer.data() + 48, testing::Ref(valueArg)))
        .WillOnce([&argTypeIdMock](void* ptr, const ValueHolder& source){ return argTypeIdMock.constructMock(ptr, &source); });
    const auto value { testData.makeValue(buffer.data()) };
    EXPECT_EQ(static_cast<void*>(value.holder), buffer.data() + 48);
    EXPECT_EQ(static_cast<ValueHolderMock*>(value.holder)->refValue, &valueArg);
    EXPECT_FALSE(value.mustDestruct);
}


const size_t mockSize = sizeof(ValueHolderMock);
const size_t mockAlign = alignof(ValueHolderMock);

constexpr TypeLayout makeTypeLayout(const size_t addedSize, const size_t alignMult, size_t sizeAlignMult)
{
    TypeLayout layout = MemoryAllocator::alignUp({ mockSize + addedSize, mockAlign * alignMult });
    assert(alignMult <= sizeAlignMult);
    const size_t sizeOneMask = sizeAlignMult * mockAlign;
    const size_t sizeZeroMask = ~(sizeOneMask - 1);
    layout.size = layout.size & sizeZeroMask | sizeOneMask;
    return layout;
}

const TypeLayout layout11 = makeTypeLayout(0, 1, 1);
const TypeLayout layout12 = makeTypeLayout(32, 1, 2);
const TypeLayout layout22 = makeTypeLayout(42, 2, 2);
const TypeLayout layout44 = makeTypeLayout(30, 4, 4);
const TypeLayout layoutRef = layout11;
const TypeLayout layoutPtr = TypeLayout::make<ValueHolder*>();

std::unordered_map<std::string, std::unique_ptr<TypeIdMock>> makeTypes()
{
    std::unordered_map<std::string, std::unique_ptr<TypeIdMock>> types;
    types["typeRef"] = TypeIdMock::makeWithCalls(layoutRef, true, nullptr);
    types["typeRef2"] = TypeIdMock::makeWithCalls(layoutRef, true, nullptr);
    TypeIdMock* refType = types["typeRef"].get();
    TypeIdMock* refType2 = types["typeRef2"].get();
    types["type11"] = TypeIdMock::makeWithCalls(layout11, false, refType);
    types["type12"] = TypeIdMock::makeWithCalls(layout12, false, refType);
    types["type22"] = TypeIdMock::makeWithCalls(layout22, false, refType2);
    types["type44"] = TypeIdMock::makeWithCalls(layout44, false, refType2);
    return types;
}

std::vector<std::unique_ptr<ValueHolderMock>> makeValues(const std::unordered_map<std::string, std::unique_ptr<TypeIdMock>>& types)
{
    std::vector<std::unique_ptr<ValueHolderMock>> values;
    for (const auto& [key, typeId] : types)
    {
        if (!key.starts_with("typeRef"))
        {
            values.push_back(std::make_unique<ValueHolderMock>(*typeId));
        }
    }
    return values;
}


class ExecutionDataBlockBaseTest : public testing::Test
{
protected:
    ExecutionDataBlockBaseTest() : typeIds(makeTypes()), sourceVals(makeValues(typeIds))
    {
        ValueHolderMock::destructList = &destructList;
    }

    ~ExecutionDataBlockBaseTest()
    {
        ValueHolderMock::destructList = nullptr;
    }

    std::unordered_map<std::string, std::unique_ptr<TypeIdMock>> typeIds;
    std::vector<std::unique_ptr<ValueHolderMock>> sourceVals;
    std::vector<const ValueHolder*> destructList;
};


template <typename DESTR_DATA>
struct DataBlockTestParams
{
    struct DataTypePlaceParams
    {
        std::string typeId;
        size_t offset;
        std::optional<size_t> valueIndex;
    };
	TypeLayout blockLayout;
	std::vector<DataTypePlaceParams> values;
	size_t valuesOffset { 0 };
    DESTR_DATA destructData;
};

class ExecutionDataBlockTest : public ExecutionDataBlockBaseTest, public testing::WithParamInterface<DataBlockTestParams<std::vector<size_t>>>
{
public:
    using Params = DataBlockTestParams<std::vector<size_t>>;

protected:
    DataBlockDef::Layout makeDataLayout(const Params& params) const
    {
        std::vector<DataTypePlace> values;
        std::transform(params.values.begin(), params.values.end(), std::back_inserter(values),
            [this](const auto& val)
            {
                const ValueHolder* sourceVal = val.valueIndex ? sourceVals[*val.valueIndex].get() : nullptr;
                return DataTypePlace{ *typeIds.at(val.typeId), val.offset, sourceVal };
            });
        return { params.blockLayout, std::move(values), params.valuesOffset, params.destructData.size() };
    }

    std::vector<ValueHolderMock*> collectAndCheckValuesFromDataBlock(const DataBlockDef::Layout& layout,
        const std::vector<Params::DataTypePlaceParams>& params, const MemoryAllocatorMock& memAlloc) const
    {
        std::vector<ValueHolderMock*> valueHolders;
        DataBlock<MemoryAllocatorMock> testBlock{ layout };
        for (size_t index = 0; index < params.size(); ++index)
        {
            auto* var = static_cast<ValueHolderMock*>(&testBlock.get(index));
            const auto& valParam = params[index];
            EXPECT_EQ(static_cast<void*>(var), memAlloc.cPtr(valParam.offset));
            EXPECT_EQ(var->getSpecTypeId(), *typeIds.at(valParam.typeId));
            EXPECT_EQ(var->refValue, valParam.valueIndex ? sourceVals[*valParam.valueIndex].get() : nullptr);
            valueHolders.push_back(var);
        }
        return valueHolders;
    }

    std::vector<const ValueHolder*> collectIndexedValues(const std::vector<size_t>& indices, const std::vector<ValueHolderMock*>& holders) const
    {
        std::vector<const ValueHolder*> filteredHolders;
        for (const size_t index : indices)
            filteredHolders.push_back(holders[index]);
        return filteredHolders;
    }
};

TEST_P(ExecutionDataBlockTest, DataBlockCreate)
{
    DataBlockDef::Layout testLayout = makeDataLayout(GetParam());
    MemoryAllocatorMock memBuff{{GetParam().blockLayout}};
    std::vector<ValueHolderMock*> vars = collectAndCheckValuesFromDataBlock(testLayout, GetParam().values, memBuff);
    EXPECT_EQ(destructList, collectIndexedValues(GetParam().destructData, vars));
}

INSTANTIATE_TEST_SUITE_P(ExecutionDataBlockParamTest, ExecutionDataBlockTest,
    testing::Values(ExecutionDataBlockTest::Params{},
        ExecutionDataBlockTest::Params{MemoryAllocator::alignUp(layout11 + layoutPtr * 2), { {"type11", 0} }, layout11.size, {0}},
        ExecutionDataBlockTest::Params{MemoryAllocator::alignUp(layout11 + layoutPtr), { {"typeRef", 0, 1} }, layoutRef.size, {}},
        ExecutionDataBlockTest::Params{MemoryAllocator::alignUp(layoutRef + layout11 + layout22 + layoutPtr * 5),
            { {"typeRef", layout22.size, 2}, {"type22", 0}, {"type11", (layout11 + layout22).size}},
            (layoutRef + layout11 + layout22).size, {1, 2}},
        ExecutionDataBlockTest::Params{MemoryAllocator::alignUp(layoutRef * 2 + layout22 + layout44 + layoutPtr * 6),
            { {"typeRef", layout22.size, 0}, {"type22", 0}, {"typeRef", (layoutRef + layout22).size, 1}, {"type44", (layoutRef * 2 + layout22).size}},
            (layoutRef * 2 + layout22 + layout44).size, {1, 3}}));



struct DataBlockDefTestParams
{
    std::vector<std::variant<std::string, int>> places;
    DataBlockTestParams<size_t> layout;
};

class ExecutionDataBlockDefTest : public ExecutionDataBlockBaseTest, public testing::WithParamInterface<DataBlockDefTestParams>
{
protected:
    void SetUp() override
    {
        ASSERT_EQ(GetParam().layout.values.size(), GetParam().places.size());
        ExecutionDataBlockBaseTest::SetUp();
        for (const auto& place : GetParam().places)
            checkRefs.push_back(place.index() == 1 ? sourceVals[std::get<1>(place)].get() : nullptr);
    }

    DataBlockDef makeDataBlockDef(const DataBlockDefTestParams& params)
    {
        DataBlockDef::Builder builder;
        for (const auto& place : params.places)
            if (place.index() == 0)
                builder.addPlace(*typeIds[std::get<0>(place)]);
            else
                builder.addPlace(std::move(sourceVals[std::get<1>(place)]));
        return builder.build();
    }

    std::vector<ValueHolderMock*> checkRefs;
};

TEST_P(ExecutionDataBlockDefTest, DataBlockDefCreate)
{
    DataBlockDef testDataDef = makeDataBlockDef(GetParam());
    EXPECT_EQ(testDataDef.layout.blockLayout, GetParam().layout.blockLayout);
    EXPECT_EQ(testDataDef.layout.values.size(), GetParam().layout.values.size());
    MemoryAllocatorMock memBuff{{GetParam().layout.blockLayout}};
    for (size_t index = 0; index < GetParam().layout.values.size(); ++index)
    {
        auto value = testDataDef.layout.values[index].makeValue(memBuff.nPtr(0));
        const auto& checkVal = GetParam().layout.values[index];
        EXPECT_EQ(static_cast<void*>(value.holder), memBuff.nPtr(checkVal.offset));
        EXPECT_EQ(value.holder->getSpecTypeId(), *typeIds[checkVal.typeId]);
        EXPECT_EQ(static_cast<ValueHolderMock*>(value.holder)->refValue, checkRefs[index]);
    }
    EXPECT_EQ(testDataDef.layout.valuesOffset, GetParam().layout.valuesOffset);
    EXPECT_EQ(testDataDef.layout.destructCount, GetParam().layout.destructData);
}

INSTANTIATE_TEST_SUITE_P(ExecutionDataBlockDefParamTest, ExecutionDataBlockDefTest,
    testing::Values(DataBlockDefTestParams{{}, {{0, layoutPtr.alignment}}},
        DataBlockDefTestParams{{"type11"}, {MemoryAllocator::alignUp(layout11 + layoutPtr * 2), {{"type11", 0}}, layout11.size, 1}},
        DataBlockDefTestParams{{0}, {MemoryAllocator::alignUp(layoutRef + layoutPtr), {{"typeRef", 0, 0}}, layout11.size, 0}},
        DataBlockDefTestParams{{1, "type12", "type11"},
            {MemoryAllocator::alignUp(layoutRef + layout12 + layout11 + layoutPtr * 5),
            {{"typeRef", 0, 1},{"type12", layoutRef.size},{"type11", (layoutRef + layout12).size}},
            (layoutRef + layout12 + layout11).size, 2}},
        DataBlockDefTestParams{{"type11", "type22"},
            {MemoryAllocator::alignUp(layout11 + layout22 + layoutPtr * 4),
            {{"type11", layout22.size},{"type22", 0}},
            (layout11 + layout22).size, 2}}));


class FunctionMock : public Function
{
public:
    MOCK_METHOD(void, execute, (ExecutionContext& context), (const, override));
};


TEST(ExecutionTest, RunCode)
{
    ExecutionContext context;

    auto fnc1 = std::make_unique<FunctionMock>();
    auto fnc2 = std::make_unique<FunctionMock>();
    auto fnc3 = std::make_unique<FunctionMock>();
    
    testing::InSequence seq;
    EXPECT_CALL(*fnc1, execute(testing::Ref(context))).Times(1);
    EXPECT_CALL(*fnc2, execute(testing::Ref(context))).Times(1);
    EXPECT_CALL(*fnc3, execute(testing::Ref(context))).Times(1);
    CodeBlock testCode{{}};
    testCode.operations.push_back(std::move(fnc1));
    testCode.operations.push_back(std::move(fnc2));
    testCode.operations.push_back(std::move(fnc3));

    DataBlock<> data;
    context.run(testCode, data);
}

TEST(ExecutionTest, RunCodeWithJumps)
{
    ExecutionContext context;

    auto fnc1 = std::make_unique<FunctionMock>();
    auto fnc2 = std::make_unique<FunctionMock>();
    auto fnc3 = std::make_unique<FunctionMock>();

    testing::InSequence seq;
    EXPECT_CALL(*fnc1, execute(testing::Ref(context))).Times(1);
    EXPECT_CALL(*fnc2, execute(testing::Ref(context)))
        .WillOnce([](ExecutionContext& cont){ cont.jump(-1); });
    EXPECT_CALL(*fnc1, execute(testing::Ref(context)))
        .WillOnce([](ExecutionContext& cont){ cont.jump(2); });
    EXPECT_CALL(*fnc3, execute(testing::Ref(context))).Times(1);
    CodeBlock testCode{{}};
    testCode.operations.push_back(std::move(fnc1));
    testCode.operations.push_back(std::move(fnc2));
    testCode.operations.push_back(std::move(fnc3));

    DataBlock<> data;
    context.run(testCode, data);
}
