#include <gmock/gmock.h>

#include <CppScript/OperationBlock.h>
#include <CppScript/CoreOperations.h>
#include "AllocatorMock.h"
#include "OperationResolverProxy.h"
#include "ValueMock.h"
#include "TestUtils.h"

using namespace CppScript;
using namespace CppScriptTest;

TypeId::Layout sumLayouts(const std::vector<const TypeId*>& types)
{
	return std::accumulate(types.begin(), types.end(), TypeId::Layout{},
		[](const TypeId::Layout& acc, const TypeId* typeId)
		{
			return acc + typeId->layout;
		});
}

TEST(OperationBlockTest, OperationBlockLayoutCreate)
{
	const OperationBlockLayout layout1{ { &Value<int>::typeId, &Value<double>::typeId }, {} };
	EXPECT_THAT(layout1.valueTypes, testing::ElementsAre(&Value<int>::typeId, &Value<double>::typeId));
	EXPECT_EQ(layout1.valuesLayout, sumLayouts(layout1.valueTypes));
	EXPECT_THAT(layout1.valuesOrder, testing::ElementsAre(0, 1));
	EXPECT_EQ(layout1.valuesDestructCount, 2);
	EXPECT_TRUE(layout1.argumentPlaces.empty());
	EXPECT_EQ(layout1.fullLayout, Alignment::alignUp(layout1.valuesLayout + OperationBlockLayout::valuePtrLayout * (2 + 2)));
	
	TypeIdMock typeMocks[]{ TypeIdMock{{ 104, 8 }, true}, TypeIdMock{{ 48, 16 }, false}, TypeIdMock{{ 36, 4 }, true}, TypeIdMock{{ 80, 16 }, true} };
	const std::vector<ValuePlace> args0{ { ValuePlace::Type::Local, 2 }, { ValuePlace::Type::Module, 0 } };
	const std::vector<ValuePlace> args1{ { ValuePlace::Type::Local, 0 }, { ValuePlace::Type::Local, 1 }, { ValuePlace::Type::Local, 3 } };
	const std::vector<ValuePlace> args2{ { ValuePlace::Type::Module, 1 } };
	const OperationBlockLayout layout2{ { &typeMocks[0], &typeMocks[1], &typeMocks[2], &typeMocks[3] },	{ &args0, &args1, &args2 } };
	EXPECT_THAT(layout2.valueTypes, testing::ElementsAre(&typeMocks[0], &typeMocks[1], &typeMocks[2], &typeMocks[3]));
	EXPECT_EQ(layout2.valuesLayout, sumLayouts(layout2.valueTypes));
	EXPECT_THAT(layout2.valuesOrder, testing::ElementsAre(1, 3, 0, 2));
	EXPECT_EQ(layout2.valuesDestructCount, 3);
	EXPECT_THAT(layout2.argumentPlaces, testing::ElementsAre(&args0, &args1, &args2));
	EXPECT_EQ(layout2.fullLayout, Alignment::alignUp(layout2.valuesLayout + OperationBlockLayout::valuePtrLayout * (4 + 3 + 6)
		+ OperationBlockLayout::argumentPtrLayout * 3));
}

TEST(OperationBlockTest, VariablesFrameInitializeAndDestruct)
{
	std::byte buffer[1024];

	ValueTypeIdMock<const TestStructDestruct&> refTypeMock;
	EXPECT_CALL(refTypeMock, create(testing::_))
		.WillOnce([](void* buff){ return new(buff) TestDestruct<Value<const TestStructDestruct&>>; });
	auto refDestructList = TestDestruct<Value<const TestStructDestruct&>>::initDestructList();
	
	const OperationBlockLayout layout{ { &Value<int>::typeId, &Value<double>::typeId, &Value<TestStructDestruct>::typeId, &refTypeMock }, {} };
	auto destructList = TestStructDestruct::initDestructList();
	TestStructDestruct* structPtr{ nullptr };
	{
		VariablesFrame frame;
		auto* memPtr = frame.initialize(buffer, layout);
		const auto fullLayout = Alignment::alignUp(layout.valuesLayout, OperationBlockLayout::valuePtrLayout.alignment)
			+ (4 + 3) * OperationBlockLayout::valuePtrLayout;
		EXPECT_EQ(memPtr, buffer + fullLayout.size);
		EXPECT_EQ(frame.getValues().size(), 4);
		ValueBase& intVal = frame.get(0);
		ValueBase& doubleVal = frame.get(1);
		ValueBase& structVal = frame.get(2);
		ValueBase& structRef = frame.get(3);
		EXPECT_EQ(&intVal, static_cast<void*>(buffer));
		EXPECT_EQ(intVal.getTypeId(), Value<int>::typeId);
		const void* doublePtr = buffer + Value<int>::typeId.layout.size;
		EXPECT_EQ(&doubleVal, doublePtr);
		EXPECT_EQ(doubleVal.getTypeId(), Value<double>::typeId);
		const void* structValPtr = static_cast<const char*>(doublePtr) + Value<double>::typeId.layout.size;
		EXPECT_EQ(&structVal, structValPtr);
		EXPECT_EQ(structVal.getTypeId(), Value<TestStructDestruct>::typeId);
		const void* structRefPtr = static_cast<const char*>(structValPtr) + Value<TestStructDestruct>::typeId.layout.size;
		EXPECT_EQ(&structRef, structRefPtr);
		EXPECT_EQ(structRef.getTypeId(), Value<const TestStructDestruct&>::typeId);

		EXPECT_EQ(destructList->size(), 0);
		auto* structValReal = static_cast<Value<TestStructDestruct>*>(&structVal);
		structValReal->set({ 123, true });
		structPtr = &structValReal->get();
		EXPECT_EQ(structPtr->val, 123);
		EXPECT_TRUE(structPtr->flag);
		EXPECT_EQ(destructList->size(), 1);
		auto& structRefReal = static_cast<Value<const TestStructDestruct&>&>(structRef);
		structRefReal.set(structValReal->get());
		EXPECT_EQ(&structRefReal.get(), structPtr);
		static_cast<Value<int>&>(intVal).set(741);
		static_cast<Value<double>&>(doubleVal).set(-12.5);
		EXPECT_EQ(static_cast<Value<int>&>(intVal).get(), 741);
		EXPECT_EQ(static_cast<Value<double>&>(doubleVal).get(), -12.5);

		VariablesFrame movedFrame{ std::move(frame) };
		EXPECT_EQ(movedFrame.getValues().size(), 4);
		EXPECT_EQ(frame.getValues().size(), 0);
	}
	EXPECT_EQ(destructList->size(), 2);
	EXPECT_EQ(structPtr, destructList->back());
	EXPECT_EQ(refDestructList->size(), 0);
}

TEST(OperationBlockTest, ConstantsFrameInitializeAndDestruct)
{
	auto destructList = TestStructDestruct::initDestructList();
	const TestStructDestruct* structPtr{ nullptr };
	{
		EXPECT_EQ(destructList->size(), 0);
		ConstantsFrame frame{ [&]()
			{
				std::vector<std::unique_ptr<ValueBase>> constVals;
				auto intVal = std::make_unique<Value<int>>();
				intVal->set(789);
				constVals.push_back(std::move(intVal));
				auto floatVal =  std::make_unique<Value<float>>();
				floatVal->set(6.28F);
				constVals.push_back(std::move(floatVal));
				auto structVal = std::make_unique<Value<TestStructDestruct>>();
				structVal->set({ 42, true });
				structPtr = &structVal->get();
				constVals.push_back(std::move(structVal));
				return constVals;
			}() };
		EXPECT_EQ(destructList->size(), 1);

		EXPECT_EQ(frame.getValues().size(), 3);
		ValueBase& intVal = frame.get(0);
		ValueBase& floatVal = frame.get(1);
		ValueBase& structVal = frame.get(2);
		EXPECT_EQ(intVal.getTypeId(), Value<int>::typeId);
		EXPECT_EQ(static_cast<Value<int>&>(intVal).get(), 789);
		EXPECT_EQ(floatVal.getTypeId(), Value<float>::typeId);
		EXPECT_EQ(static_cast<Value<float>&>(floatVal).get(), 6.28F);
		EXPECT_EQ(structVal.getTypeId(), Value<TestStructDestruct>::typeId);
		EXPECT_EQ(&static_cast<Value<TestStructDestruct>&>(structVal).get(), structPtr);
		EXPECT_EQ(static_cast<Value<TestStructDestruct>&>(structVal).get().val, 42);
		EXPECT_TRUE(static_cast<Value<TestStructDestruct>&>(structVal).get().flag);

		ConstantsFrame movedFrame{ std::move(frame) };
		EXPECT_EQ(movedFrame.getValues().size(), 3);
		EXPECT_EQ(frame.getValues().size(), 0);
	}
	EXPECT_EQ(destructList->size(), 2);
	EXPECT_EQ(structPtr, destructList->back());
}

TEST(OperationBlockTest, OperationsArgumentsFrameInitialize)
{
	const std::vector<ValuePlace> op0Args{ {ValuePlace::Type::Local, 0} };
	const std::vector<ValuePlace> op1Args{ {ValuePlace::Type::Module, 1}, {ValuePlace::Type::Caller, 0} };
	const std::vector<ValuePlace> op2Args{ {ValuePlace::Type::Module, 0}, { ValuePlace::Type::Local, 0} };
	const OperationBlockLayout localLayout{ { &Value<float>::typeId }, {&op0Args, &op1Args, &op2Args } };
	
	std::byte buffer[1024];
	VariablesFrame moduleFrame, callerFrame, localFrame;
	std::byte* nextBuff = moduleFrame.initialize(buffer, OperationBlockLayout({ &Value<int>::typeId, &Value<TestStruct>::typeId }, {}));
	auto& intVal = static_cast<Value<int>&>(moduleFrame.get(0));
	intVal.set(963);
	auto& structVal = static_cast<Value<TestStruct>&>(moduleFrame.get(1));
	structVal.set({ 321, false });
	nextBuff = localFrame.initialize(nextBuff, localLayout);
	auto& floatVal = static_cast<Value<float>&>(localFrame.get(0));
	floatVal.set(456.75f);
	nextBuff = callerFrame.initialize(nextBuff, OperationBlockLayout({ &Value<bool>::typeId }, {}));
	auto& boolVal = static_cast<Value<bool>&>(callerFrame.get(0));
	boolVal.set(true);
	OperationBlockContext opBlockContext{{localFrame.getValues(),  callerFrame.getValues(), moduleFrame.getValues() }};
	
	OperationsArgumentsFrame argFrame;
	std::byte* buffLeft = argFrame.initialize(nextBuff, localLayout, opBlockContext);
	EXPECT_EQ(buffLeft, nextBuff + OperationBlockLayout::argumentPtrLayout.size * 3
		+ OperationBlockLayout::valuePtrLayout.size * 5);
	Arguments op0ArgVals = argFrame.get(0);
	EXPECT_EQ(op0ArgVals[0], &floatVal);
	Arguments op1ArgVals = argFrame.get(1);
	EXPECT_EQ(op1ArgVals[0], &structVal);
	EXPECT_EQ(op1ArgVals[1], &boolVal);
	Arguments op2ArgVals = argFrame.get(2);
	EXPECT_EQ(op2ArgVals[0], &intVal);
	EXPECT_EQ(op2ArgVals[1], &floatVal);
}
	
TEST(OperationBlockTest, OperationFrameCreateExecute)
{
	CoreOperationBuilder builder;
	TypeFrames frames;
	OperationBlockResolutionData opData{{}, makeConstants(StdInt(0), StdInt(1), StdInt(10)),
		{{{"="}, ValuePlace{ValuePlace::Type::Local, 0}, {{ValuePlace::Type::Constants, 0}}},
		{{"+="}, {}, {{ValuePlace::Type::Local, 0}, {ValuePlace::Type::Constants, 1}}},
		{{"<=>"}, {}, {{ValuePlace::Type::Local, 0}, {ValuePlace::Type::Constants, 2}}, {-1, 1, 1}}}};
	OperationBlock<CoreOperationBuilder> opBlock{ builder, std::get<0>(builder.getResolver().resolve(std::move(opData), frames))};
	OperationBlockContext context = opBlock.createContext();
	EXPECT_EQ(getFrame(context.frames, ValuePlace::Type::Constants).size(), 3);

	std::byte buffer[1024];
	AllocMock allocMock;
	TypeId::Layout fullLayout = Value<StdInt>::typeId.layout + OperationBlockLayout::valuePtrLayout * ((1 + 1) + 3 * 2)
		+ OperationBlockLayout::argumentPtrLayout * 3;
	allocMock.expectAllocFree(fullLayout, buffer, false);
	auto opFrame{ opBlock.createFrame<AllocMock>(context) };
	opFrame.execute();
	EXPECT_EQ(static_cast<Value<StdInt>&>(opFrame.getValues().get(0)).get(), 10);
}
