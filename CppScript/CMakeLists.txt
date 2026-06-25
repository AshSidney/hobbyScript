cmake_minimum_required(VERSION 3.1)
project(CppScript CXX)

set(CMAKE_CXX_STANDARD 20)

if (${BUILD_SHARED_LIBS})
    add_compile_definitions(CPPSCRIPT_DLL)
else ()
	add_compile_definitions(_NOEXPORT)
endif ()

find_package(pegtl REQUIRED)
find_package(GTest REQUIRED)

include_directories(include source)

add_library(CppScript
    include/CppScript/Definitions.h
    include/CppScript/Value.h
    include/CppScript/CoreOperations.h
    include/CppScript/Operation.h
    include/CppScript/OperationBlock.h
    include/CppScript/OperationResolver.h
    include/CppScript/Module.h
    include/CppScript/Allocator.h
    include/CppScript/EnumTraits.h
    include/CppScript/IntValue.h
    include/CppScript/Parser.h
    include/CppScript/ParserUtils.h

    include/CppScript/Execution.h
    include/CppScript/FunctionDef.h
    include/CppScript/ScriptModule.h
    include/CppScript/CoreModule.h
    include/CppScript/ValueHolder.h

    source/Value.cpp
    source/IntValue.cpp
    source/EnumTraits.cpp
    source/OperationBlock.cpp
    source/OperationResolver.cpp
    source/Allocator.cpp
    source/Parser.cpp
    source/ParserUtils.cpp
    source/IntUtils.h
    source/IntUtils.cpp

    source/Execution.cpp
    source/FunctionDef.cpp
    source/CoreModule.cpp
    source/ScriptModule.cpp
    source/ValueHolder.cpp
   )

target_link_libraries(CppScript
    PUBLIC
    taocpp::pegtl
    )

add_executable(CppScriptTest
    test/CppScriptTest.cpp
    test/ValueTest.cpp
    test/OperationTest.cpp
    test/OperationBlockTest.cpp
    test/OperationResolverTest.cpp
    test/ModuleTest.cpp
    test/AllocatorTest.cpp
    test/EnumTraitsTest.cpp
    test/IntValueTest.cpp
    test/IntUtilsTest.cpp
    test/CoreOperationsTest.cpp
    test/ParserTest.cpp
    test/ParserUtilsTest.h
    test/ParserUtilsTest.cpp
    test/AllocatorMock.h
    test/AllocatorMock.cpp
    test/OperationResolverProxy.h
    test/OperationResolverProxy.cpp
    test/ValueMock.h
    test/ValueMock.cpp
    test/PerfAlgorithms.h
    test/TestUtils.h
    test/TestUtils.cpp
   
    test/ExecutionTest.cpp
    test/FunctionDefTest.cpp
    test/CoreModuleTest.cpp
    test/ValueHolderTest.cpp
    test/ScriptModuleTest.cpp
   )

target_link_libraries(CppScriptTest
    CppScript
    gtest::gtest
    )

target_compile_definitions(CppScriptTest PUBLIC TESTING_ENABLED)
