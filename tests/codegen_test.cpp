#include <gtest/gtest.h>
#include <sstream>
#include "lexer.h"
#include "parser.h"
#include "codegen.h"

static std::unique_ptr<Program> parseString(const std::string& code) {
    std::istringstream input(code);
    Lexer lexer(input);
    Parser parser(lexer);
    return parser.parseProgram();
}

TEST(CodeGenTest, SimpleFunction) {
    auto program = parseString("fn main() -> i32 { return 0; }");
    CodeGenerator codegen;
    EXPECT_NO_THROW(codegen.generate(*program, "/tmp/slang_test_simple"));
}

TEST(CodeGenTest, LetAndReturn) {
    auto program = parseString(R"(
        fn main() -> i32 {
            let x: i32 = 42;
            return x;
        }
    )");
    CodeGenerator codegen;
    EXPECT_NO_THROW(codegen.generate(*program, "/tmp/slang_test_let"));
}

TEST(CodeGenTest, IfElse) {
    auto program = parseString(R"(
        fn main() -> i32 {
            let x: i32 = 5;
            if x > 3 {
                return 1;
            } else {
                return 0;
            }
        }
    )");
    CodeGenerator codegen;
    EXPECT_NO_THROW(codegen.generate(*program, "/tmp/slang_test_ifelse"));
}

TEST(CodeGenTest, WhileLoop) {
    auto program = parseString(R"(
        fn main() -> i32 {
            let mut i: i32 = 0;
            let mut sum: i32 = 0;
            while i < 10 {
                sum = sum + i;
                i = i + 1;
            }
            return sum;
        }
    )");
    CodeGenerator codegen;
    EXPECT_NO_THROW(codegen.generate(*program, "/tmp/slang_test_while"));
}

TEST(CodeGenTest, FunctionCalls) {
    auto program = parseString(R"(
        fn add(a: i32, b: i32) -> i32 {
            return a + b;
        }
        fn main() -> i32 {
            return add(3, 4);
        }
    )");
    CodeGenerator codegen;
    EXPECT_NO_THROW(codegen.generate(*program, "/tmp/slang_test_calls"));
}
