#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <CppScript/Execution.h>
#include <CppScript/FunctionDef.h>
#include <numeric>
#include <unordered_map>

#include "TestUtils.h"

using namespace CppScript;

class ValueHolderMock;

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

    ValueHolderMock* constructMock(void* ptr);
    ValueHolderMock* constructRefMock(void* ptr, const ValueHolder& source);

    static std::unique_ptr<TypeIdMock> makeWithCalls(const TypeLayout& typeLayout, const bool isRef, const TypeIdMock* refType);
};

class ValueHolderMock : public ValueHolder
{
public:
    ~ValueHolderMock()
    {
        destructList.push_back(this);
    }

    //MOCK_METHOD(void, set, (const ValueHolder& source), (override));
    //MOCK_METHOD(void, setRef, (const ValueHolder& source), (override));
    MOCK_METHOD(const TypeId&, getTypeId, (), (const, override));
    MOCK_METHOD(const TypeId&, getSpecTypeId, (), (const, override));

    const ValueHolder* ref { nullptr };

    static std::vector<const ValueHolder*> destructList;
};

std::vector<const ValueHolder*> ValueHolderMock::destructList;


ValueHolderMock* TypeIdMock::constructMock(void* ptr)
{
    auto* holder = new(ptr) ValueHolderMock;
    EXPECT_CALL(*holder, getSpecTypeId())
        .WillRepeatedly(testing::ReturnRef(*this));
    return holder;
}

ValueHolderMock* TypeIdMock::constructRefMock(void* ptr, const ValueHolder& source)
{
    auto* holder = constructMock(ptr);
    holder->ref = &source;
    return holder;
}

std::unique_ptr<TypeIdMock> TypeIdMock::makeWithCalls(const TypeLayout& layout, const bool isRef, const TypeIdMock* refType)
{
    auto holder = std::make_unique<TypeIdMock>(layout, isRef, refType);
    EXPECT_CALL(*holder, construct(testing::_))
        .WillRepeatedly(testing::Invoke(holder.get(), &TypeIdMock::constructMock));
    if (isRef)
    {
        EXPECT_CALL(*holder, constructRef(testing::_, testing::_))
            .WillRepeatedly(testing::Invoke(holder.get(), &TypeIdMock::constructRefMock));
    }
    return holder;
}


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

    std::byte* cPtr(const size_t offset)
    {
        return currBuffer->buffer.data() + offset;
    }

    std::byte* nPtr(const size_t offset)
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


class FunctionMock : public Function
{
public:
    MOCK_METHOD(void, execute, (ExecutionContext& context), (const, override));
};


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
    TypeIdMock typeIdMock{{}, false, nullptr};
    std::array<std::byte, 1024> buffer;
    const DataTypePlace testData{typeIdMock, 16, nullptr};
    EXPECT_CALL(typeIdMock, construct(buffer.data() + 16))
        .WillOnce(testing::Invoke(&typeIdMock, &TypeIdMock::constructMock));
    auto value = testData.makeValue(buffer.data());
    EXPECT_EQ(static_cast<void*>(value.holder), buffer.data() + 16);
    EXPECT_TRUE(value.mustDestruct);
}

TEST(ExecutionTest, DataTypePlace_MakeValueRef)
{
    TypeIdMock typeIdMock{{}, true, nullptr};
    std::array<std::byte, 1024> buffer;
    ValueHolderMock valueSource;
    const DataTypePlace testData{typeIdMock, 32, &valueSource};
    EXPECT_CALL(typeIdMock, constructRef(buffer.data() + 32, testing::Ref(valueSource)))
        .WillOnce(testing::Invoke(&typeIdMock, &TypeIdMock::constructRefMock));
    auto value = testData.makeValue(buffer.data());
    EXPECT_EQ(static_cast<void*>(value.holder), buffer.data() + 32);
    EXPECT_EQ(static_cast<ValueHolderMock*>(value.holder)->ref, &valueSource);
    EXPECT_FALSE(value.mustDestruct);
}

TEST(ExecutionTest, DataTypePlace_MakeValueArg)
{
    TypeIdMock typeIdMock{{}, true, nullptr};
    std::array<std::byte, 1024> buffer;
    const DataTypePlace testData{typeIdMock, 48, nullptr};
    ValueHolderMock valueArg;
    testData.setArgument(valueArg);
    EXPECT_CALL(typeIdMock, constructRef(buffer.data() + 48, testing::Ref(valueArg)))
        .WillOnce(testing::Invoke(&typeIdMock, &TypeIdMock::constructRefMock));
    auto value = testData.makeValue(buffer.data());
    EXPECT_EQ(static_cast<void*>(value.holder), buffer.data() + 48);
    EXPECT_EQ(static_cast<ValueHolderMock*>(value.holder)->ref, &valueArg);
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
            auto value = std::make_unique<ValueHolderMock>();
            EXPECT_CALL(*value, getSpecTypeId())
                .WillRepeatedly(testing::ReturnRef(*typeId));
            values.push_back(std::move(value));
        }
    }
    return values;
}


class ExecutionDataBlockBaseTest : public testing::Test
{
protected:
    void SetUp() override
    {
        typeIds = makeTypes();
        sourceVals = makeValues(typeIds);
        ValueHolderMock::destructList.clear();
    }

    std::unordered_map<std::string, std::unique_ptr<TypeIdMock>> typeIds;
    std::vector<std::unique_ptr<ValueHolderMock>> sourceVals;
};


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
	size_t destructCount { 0 };

    std::vector<size_t> destrIndices;
};

class ExecutionDataBlockTest : public ExecutionDataBlockBaseTest, public testing::WithParamInterface<DataBlockTestParams>
{
protected:
    DataBlockDef::Layout makeDataLayout(const DataBlockTestParams& params)
    {
        std::vector<DataTypePlace> values;
        std::transform(params.values.begin(), params.values.end(), std::back_inserter(values),
            [this](const auto& val)
            {
                const ValueHolder* sourceVal = val.valueIndex ? sourceVals[*val.valueIndex].get() : nullptr;
                return DataTypePlace{ *typeIds[val.typeId], val.offset, sourceVal };
            });
        return { params.blockLayout, std::move(values), params.valuesOffset, params.destructCount };
    }
};

TEST_P(ExecutionDataBlockTest, DataBlockCreate)
{
    DataBlockDef::Layout testLayout = makeDataLayout(GetParam());
    MemoryAllocatorMock memBuff{{GetParam().blockLayout}};
    std::vector<ValueHolderMock*> vars;
    {
        DataBlock<MemoryAllocatorMock> testBlock{ testLayout };
        for (size_t index = 0; index < GetParam().values.size(); ++index)
        {
            auto* var = static_cast<ValueHolderMock*>(&testBlock.get(index));
            const auto& valParam = GetParam().values[index];
            EXPECT_EQ(static_cast<void*>(var), memBuff.cPtr(valParam.offset));
            EXPECT_EQ(var->getSpecTypeId(), *typeIds[valParam.typeId]);
            EXPECT_EQ(var->ref, valParam.valueIndex ? sourceVals[*valParam.valueIndex].get() : nullptr);
            vars.push_back(var);
        }
    }
    std::vector<const ValueHolder*> expectedDestructs;
    for (const size_t index : GetParam().destrIndices)
        expectedDestructs.push_back(vars[index]);
    EXPECT_EQ(ValueHolderMock::destructList, expectedDestructs);
}

INSTANTIATE_TEST_SUITE_P(ExecutionDataBlockParamTest, ExecutionDataBlockTest,
    testing::Values(DataBlockTestParams{},
        DataBlockTestParams{MemoryAllocator::alignUp(layout11 + layoutPtr * 2), { {"type11", 0} }, layout11.size, 1, {0}},
        DataBlockTestParams{MemoryAllocator::alignUp(layout11 + layoutPtr), { {"typeRef", 0, 1} }, layoutRef.size, 0, {}},
        DataBlockTestParams{MemoryAllocator::alignUp(layoutRef + layout11 + layout22 + layoutPtr * 5),
            { {"typeRef", layout22.size, 2}, {"type22", 0}, {"type11", (layout11 + layout22).size}},
            (layoutRef + layout11 + layout22).size, 2, {1, 2}},
        DataBlockTestParams{MemoryAllocator::alignUp(layoutRef * 2 + layout22 + layout44 + layoutPtr * 6),
            { {"typeRef", layout22.size, 0}, {"type22", 0}, {"typeRef", (layoutRef + layout22).size, 1}, {"type44", (layoutRef * 2 + layout22).size}},
            (layoutRef * 2 + layout22 + layout44).size, 2, {1, 3}}));


struct DataBlockDefTestParams
{
    std::vector<std::variant<std::string, int>> places;
    DataBlockTestParams layout;
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
        EXPECT_EQ(static_cast<ValueHolderMock*>(value.holder)->ref, checkRefs[index]);
    }
    EXPECT_EQ(testDataDef.layout.valuesOffset, GetParam().layout.valuesOffset);
    EXPECT_EQ(testDataDef.layout.destructCount, GetParam().layout.destructCount);
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


struct TypePlaceTestParams
{
    struct ValueParams
    {
        std::string typeId;
        size_t valueOffset;
    };
    std::vector<ValueParams> values;

    TypeLayout allocLayout;
};

class ExecutionDataBlockTestOld : public testing::TestWithParam<TypePlaceTestParams>
{
public:
    void SetUp() override
    {
        types = makeTypes();
        ValueHolderMock::destructList.clear();
    }

    std::unordered_map<std::string, std::unique_ptr<TypeIdMock>> types;
};

TEST_P(ExecutionDataBlockTestOld, MakeMemoryBlock)
{
    DataBlockOld testData{ PlaceType::Local };

    std::vector<PlaceData> places;

    for (const auto& valueParam : GetParam().values)
    {
        places.push_back(testData.addPlaceType(*types[valueParam.typeId]));
        EXPECT_EQ(places.back(), PlaceData(PlaceType::Local, places.size() - 1));
    }
    MemoryAllocatorMock memBuff{ { GetParam().allocLayout } };
    std::vector<const ValueHolder*> holders;
    {
        const auto memBlock = testData.makeMemoryBlock<MemoryAllocatorMock>();
        for (const auto& place : places)
        {
            holders.push_back(&memBlock.get(place.index));
        }
        const ValueHolder* firstHolder{ nullptr };
        if (!holders.empty())
            firstHolder = std::accumulate(std::next(holders.cbegin()), holders.cend(), holders.front(),
                [](const ValueHolder* minPtr, const ValueHolder* ptr)
                {
                    return std::min(minPtr, ptr);
                });
        for (size_t idx = 0; idx < places.size(); ++idx)
        {
            const auto& valParam = GetParam().values[idx];
            EXPECT_EQ(holders[idx]->getSpecTypeId(), *types[valParam.typeId]);
            EXPECT_EQ(size_t(holders[idx]) - size_t(firstHolder), valParam.valueOffset);
        }
        if (GetParam().allocLayout.size == 0)
        {
            EXPECT_TRUE(memBuff.allocBuffers.empty());
        }
        else
        {
            ASSERT_EQ(memBuff.allocBuffers.size(), 1);
            const auto* allocMem = memBuff.allocBuffers.begin()->second;
            EXPECT_EQ(allocMem->layout, GetParam().allocLayout);
            EXPECT_FALSE(allocMem->isFree);
        }
    }
    EXPECT_EQ(holders, ValueHolderMock::destructList);
    EXPECT_TRUE(memBuff.allocBuffers.empty());
}

INSTANTIATE_TEST_SUITE_P(MemoryBlockInstances, ExecutionDataBlockTestOld,
    testing::Values(TypePlaceTestParams{},
        TypePlaceTestParams{{{"type11", 0}, {"type12", layout11.size}, {"type11", layout11.size + layout12.size}},
            MemoryAllocator::alignUp(layout11 * 2 + layout12)},
        TypePlaceTestParams{{{"type11", layout22.size}, {"type22", 0}},
            MemoryAllocator::alignUp(layout11 + layout22)},
        TypePlaceTestParams{{{"type11", layout44.size + layout22.size}, {"type22", layout44.size}, {"type44", 0},
                {"type11", layout44.size + layout22.size + layout11.size}, {"type44", layout44.size + layout22.size + 2 * layout11.size}},
            MemoryAllocator::alignUp((layout11 + layout44) * 2 + layout22)},
        TypePlaceTestParams{{{"type22", 0}, {"type12", layout22.size}, {"type44", layout12.size + layout22.size}},
            MemoryAllocator::alignUp(layout12 + layout22 + layout44)}));


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
