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
    include/CppScript/ValueHolder.h
    source/IntValue.cpp
    source/Execution.cpp
    source/IntUtils.h
    source/IntUtils.cpp
    )

add_executable(cppScriptTest
    test/CppScriptTest.cpp
    test/IntValueTest.cpp
    test/IntUtilsTest.cpp
    test/FunctionDefTest.cpp
    )

target_link_libraries(cppScriptTest
    cppScript
    CONAN_PKG::gtest
    )