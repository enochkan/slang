cmake_minimum_required(VERSION 3.14)
project(SLangCompiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set the LLVM installation path
set(LLVM_ROOT "/opt/homebrew/opt/llvm")

# Find LLVM package
list(APPEND CMAKE_PREFIX_PATH "${LLVM_ROOT}")
find_package(LLVM REQUIRED CONFIG)

include_directories(include ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Shared library with all compiler sources (used by both compiler and tests)
add_library(slang_lib STATIC
    src/lexer.cpp
    src/parser.cpp
    src/codegen.cpp
    src/ast.cpp
    src/utils.cpp
)
target_include_directories(slang_lib PUBLIC include ${LLVM_INCLUDE_DIRS})
target_compile_definitions(slang_lib PUBLIC ${LLVM_DEFINITIONS})

# Link LLVM monolithic shared library
target_link_directories(slang_lib PUBLIC ${LLVM_ROOT}/lib)
target_link_libraries(slang_lib PUBLIC LLVM)

# Compiler executable
add_executable(slangcc src/main.cpp)
target_link_libraries(slangcc slang_lib)

# --- GoogleTest ---
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(slang_tests
    tests/lexer_test.cpp
    tests/parser_test.cpp
    tests/codegen_test.cpp
)
target_link_libraries(slang_tests slang_lib gtest_main)

include(GoogleTest)
gtest_discover_tests(slang_tests)
