#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <CppScript/Execution.h>
#include <CppScript/FunctionDef.h>
#include <numeric>
#include <unordered_map>

#include "TestUtils.h"

using namespace CppScript;

class TypeIdMock : public TypeId
{
public:
    MOCK_METHOD(ValueHolder*, construct, (void* ptr), (const, override));
};

class ValueHolderMock : public ValueHolder
{
public:
    ~ValueHolderMock()
    {
        destructList.push_back(this);
    }

    MOCK_METHOD(ValueHolder*, constructRef, (void* ptr), (const, override));
    MOCK_METHOD(const TypeId&, getTypeId, (), (const, override));

    static std::vector<const ValueHolder*> destructList;
};

std::vector<const ValueHolder*> ValueHolderMock::destructList;


class MemoryAllocatorMock : public MemoryAllocator
{
public:
    struct AllocMem
    {
        TypeLayout layout;
        bool isFree{true};
    };
    static std::unordered_map<std::byte*, AllocMem> allocMem;

	static std::byte* allocate(const TypeLayout& layout)
    {
        std::byte* memPtr = MemoryAllocator::allocate(layout);
        auto& mem = allocMem[memPtr];
        EXPECT_TRUE(mem.isFree);
        mem.layout = layout;
        mem.isFree = false;
        return memPtr;
    }

	static void free(std::byte* memPtr)
    {
        auto& mem = allocMem[memPtr];
        EXPECT_FALSE(mem.isFree);
        mem.isFree = true;
        MemoryAllocator::free(memPtr);
    }
};

std::unordered_map<std::byte*, MemoryAllocatorMock::AllocMem> MemoryAllocatorMock::allocMem;


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

const TypeLayout type11 = makeTypeLayout(0, 1, 1);
const TypeLayout type12 = makeTypeLayout(32, 1, 2);
const TypeLayout type22 = makeTypeLayout(42, 2, 2);
const TypeLayout type44 = makeTypeLayout(30, 4, 4);

struct TypePlaceTestParams
{
    std::vector<TypeLayout> types;

    struct ValueParams
    {
        size_t typeIndex;
        size_t valueOffset;
    };
    std::vector<ValueParams> values;

    TypeLayout allocLayout;
};

class ExecutionDataBlockTest : public testing::TestWithParam<TypePlaceTestParams>
{
public:
    void SetUp() override
    {
        ValueHolderMock::destructList.clear();
        MemoryAllocatorMock::allocMem.clear();
        const auto& typeParams = GetParam().types;
        types = std::make_unique<TypeIdMock[]>(typeParams.size());
        for (size_t idx = 0; idx < typeParams.size(); ++idx)
        {
            auto& currType = types[idx];
            currType.layout = typeParams[idx];
            EXPECT_CALL(currType, construct(testing::_))
                .WillRepeatedly([&currType](void* ptr)
                    {
                        ValueHolderMock* holder = new(ptr) ValueHolderMock;
                        EXPECT_CALL(*holder, getTypeId())
                            .WillOnce(testing::ReturnRef(currType));
                        return holder;
                    });
        }
    }

    std::unique_ptr<TypeIdMock[]> types;
};

TEST_P(ExecutionDataBlockTest, MakeMemoryBlock)
{
    DataBlock testData{ PlaceType::Local };

    std::vector<PlaceData> places;

    for (const auto& valueParam : GetParam().values)
    {
        places.push_back(testData.addPlaceType(types[valueParam.typeIndex]));
        EXPECT_EQ(places.back(), PlaceData(PlaceType::Local, places.size() - 1));
    }
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
            EXPECT_EQ(holders[idx]->getTypeId(), types[valParam.typeIndex]);
            EXPECT_EQ(size_t(holders[idx]) - size_t(firstHolder), valParam.valueOffset);
        }
        if (GetParam().allocLayout.size == 0)
        {
            EXPECT_TRUE(MemoryAllocatorMock::allocMem.empty());
        }
        else
        {
            ASSERT_EQ(MemoryAllocatorMock::allocMem.size(), 1);
            const auto& allocMem = MemoryAllocatorMock::allocMem.begin()->second;
            EXPECT_EQ(allocMem.layout, GetParam().allocLayout);
            EXPECT_FALSE(allocMem.isFree);
        }
    }
    EXPECT_EQ(holders, ValueHolderMock::destructList);
    EXPECT_TRUE(GetParam().allocLayout.size == 0 || MemoryAllocatorMock::allocMem.begin()->second.isFree);
}

INSTANTIATE_TEST_SUITE_P(MemoryBlockInstances, ExecutionDataBlockTest,
    testing::Values(TypePlaceTestParams{},
        TypePlaceTestParams{{type11, type12}, {{0, 0}, {1, type11.size}, {0, type11.size + type12.size}},
            MemoryAllocator::alignUp(type11 * 2 + type12)},
        TypePlaceTestParams{{type11,  type22}, {{0, type22.size}, {1, 0}},
            MemoryAllocator::alignUp(type11 + type22)},
        TypePlaceTestParams{{type11, type22, type44},
            {{0, type44.size + type22.size}, {1, type44.size}, {2, 0},
                {0, type44.size + type22.size + type11.size}, {2, type44.size + type22.size + 2 * type11.size}},
            MemoryAllocator::alignUp((type11 + type44) * 2 + type22)},
        TypePlaceTestParams{{type12, type22, type44}, {{1, 0}, {0, type22.size}, {2, type12.size + type22.size}},
            MemoryAllocator::alignUp(type12 + type22 + type44)}));


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
    CodeBlock testCode;
    testCode.operations.push_back(std::move(fnc1));
    testCode.operations.push_back(std::move(fnc2));
    testCode.operations.push_back(std::move(fnc3));

    context.run(testCode);
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
    CodeBlock testCode;
    testCode.operations.push_back(std::move(fnc1));
    testCode.operations.push_back(std::move(fnc2));
    testCode.operations.push_back(std::move(fnc3));

    context.run(testCode);
}
