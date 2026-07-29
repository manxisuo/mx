cmake_minimum_required(VERSION 3.16)

project(mx
    VERSION 0.1.0
    DESCRIPTION "Header-only C++17 serialization and byte conversion helpers for Qt types"
    LANGUAGES CXX
)

add_library(mx INTERFACE)
add_library(mx::mx ALIAS mx)

target_compile_features(mx INTERFACE cxx_std_17)
target_include_directories(mx
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

find_package(Qt5 COMPONENTS Core REQUIRED)
target_link_libraries(mx INTERFACE Qt5::Core)
target_compile_options(mx INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
    $<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>
)

option(MX_BUILD_EXAMPLES "Build mx examples" OFF)
option(MX_BUILD_TESTS "Build mx tests" OFF)

if(MX_BUILD_EXAMPLES)
    add_executable(mx_basic_usage examples/basic_usage.cpp)
    target_link_libraries(mx_basic_usage PRIVATE mx::mx)
endif()

if(MX_BUILD_TESTS)
    enable_testing()
    add_executable(mx_compile_tests tests/compile_tests.cpp)
    target_link_libraries(mx_compile_tests PRIVATE mx::mx)
    add_test(NAME mx_compile_tests COMMAND mx_compile_tests)
endif()
