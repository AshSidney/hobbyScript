cmake_minimum_required(VERSION 3.1)
project(CppScriptPackageTest CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

include_directories(CppScript/include)

add_executable(CppScriptPackageTest PackageTest.cpp)
target_link_libraries(CppScriptPackageTest ${CONAN_LIBS})

# CTest is a testing tool that can be used to test your project.
# enable_testing()
# add_test(NAME example
#          WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
#          COMMAND example)
