cmake_minimum_required(VERSION 3.1)
project(CppScript CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(CMAKE_CXX_STANDARD 20)

if (${BUILD_SHARED_LIBS})
    add_compile_definitions(CPPSCRIPT_DLL)
else ()
	add_compile_definitions(_NOEXPORT)
endif ()

include_directories(include source)

add_library(cppScript
    include/CppScript/Definitions.h
    include/CppScript/Value.h
    include/CppScript/CoreOperations.h
    include/CppScript/Operation.h
    include/CppScript/OperationBlock.h
    include/CppScript/OperationResolver.h
    include/CppScript/Allocator.h
    include/CppScript/EnumTraits.h
    include/CppScript/IntValue.h

    include/CppScript/Execution.h
    include/CppScript/FunctionDef.h
    include/CppScript/ScriptModule.h
    include/CppScript/CoreModule.h
    include/CppScript/ValueHolder.h

    source/IntValue.cpp
    source/EnumTraits.cpp
    source/OperationBlock.cpp
    source/OperationResolver.cpp
    source/Allocator.cpp
    source/IntUtils.h
    source/IntUtils.cpp

    source/Execution.cpp
    source/FunctionDef.cpp
    source/CoreModule.cpp
    source/ScriptModule.cpp
    source/ValueHolder.cpp
   )

add_executable(cppScriptTest
    test/CppScriptTest.cpp
    test/ValueTest.cpp
    test/OperationTest.cpp
    test/OperationBlockTest.cpp
    test/OperationResolverTest.cpp
    test/AllocatorTest.cpp
    test/EnumTraitsTest.cpp
    test/IntValueTest.cpp
    test/IntUtilsTest.cpp
    test/CoreOperationsTest.cpp
    test/AllocatorMock.h
    test/AllocatorMock.cpp
    test/ValueMock.h
    test/ValueMock.cpp
    test/Fibonacci.h
    test/TestUtils.h
    test/TestUtils.cpp
   
    test/ExecutionTest.cpp
    test/FunctionDefTest.cpp
    test/CoreModuleTest.cpp
    test/ValueHolderTest.cpp
    test/ScriptModuleTest.cpp
   )

target_link_libraries(cppScriptTest
    cppScript
    CONAN_PKG::gtest
    )