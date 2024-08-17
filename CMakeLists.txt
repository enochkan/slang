cmake_minimum_required(VERSION 3.10)
project(SLangCompiler)

set(CMAKE_CXX_STANDARD 17)

# Set the LLVM installation path
set(LLVM_ROOT "/usr/local/opt/llvm")  # Replace with $(brew --prefix llvm) output

# Add LLVM's include directory and binary directory
include_directories(${LLVM_ROOT}/include)
link_directories(${LLVM_ROOT}/lib)

# Find LLVM package
find_package(LLVM REQUIRED CONFIG)

# Include the LLVM directories
include_directories(include ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Add executable and source files
add_executable(slangcc
    src/main.cpp
    src/lexer.cpp
    src/parser.cpp
    src/codegen.cpp
    src/ast.cpp
    src/utils.cpp
)

# Link LLVM libraries
llvm_map_components_to_libnames(llvm_libs support core irreader nativecodegen)
target_link_libraries(slangcc ${llvm_libs})
