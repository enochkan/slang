cmake_minimum_required(VERSION 3.10)
project(SLangCompiler)

set(CMAKE_CXX_STANDARD 17)
find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(slangcc src/main.cpp src/lexer.cpp src/parser.cpp src/codegen.cpp src/ast.cpp src/utils.cpp)

target_link_libraries(slangcc LLVM)
