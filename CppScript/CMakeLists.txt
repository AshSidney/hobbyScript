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
    include/CppScript/IntValue.h
    include/CppScript/Execution.h
    include/CppScript/FunctionDef.h
    include/CppScript/CoreModule.h
    include/CppScript/ValueHolder.h
    source/IntValue.cpp
    source/Execution.cpp
    source/FunctionDef.cpp
    source/CoreModule.cpp
    source/IntUtils.h
    source/IntUtils.cpp
    )

add_executable(cppScriptTest
    test/CppScriptTest.cpp
    test/ExecutionTest.cpp
    test/IntValueTest.cpp
    test/IntUtilsTest.cpp
    test/FunctionDefTest.cpp
    test/CoreModuleTest.cpp
    test/ValueHolderTest.cpp
    test/Fibonacci.h
    )

target_link_libraries(cppScriptTest
    cppScript
    CONAN_PKG::gtest
    )