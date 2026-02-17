#include <gtest/gtest.h>
#include <sstream>
#include "lexer.h"
#include "parser.h"

static std::unique_ptr<Program> parseString(const std::string& code) {
    std::istringstream input(code);
    Lexer lexer(input);
    Parser parser(lexer);
    return parser.parseProgram();
}

TEST(ParserTest, IntLiteral) {
    auto program = parseString("fn main() -> i32 { return 42; }");
    ASSERT_EQ(program->functions.size(), 1u);
    EXPECT_EQ(program->functions[0]->name, "main");

    auto& stmts = program->functions[0]->body->statements;
    ASSERT_EQ(stmts.size(), 1u);
    auto* ret = dynamic_cast<ReturnStmt*>(stmts[0].get());
    ASSERT_NE(ret, nullptr);
    auto* lit = dynamic_cast<IntLiteralExpr*>(ret->value.get());
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->value, 42);
}

TEST(ParserTest, OperatorPrecedence) {
    // 2 + 3 * 4 should parse as +(2, *(3, 4))
    auto program = parseString("fn f() -> i32 { return 2 + 3 * 4; }");
    auto& stmts = program->functions[0]->body->statements;
    auto* ret = dynamic_cast<ReturnStmt*>(stmts[0].get());
    auto* add = dynamic_cast<BinaryExpr*>(ret->value.get());
    ASSERT_NE(add, nullptr);
    EXPECT_EQ(add->op, "+");

    auto* lhs = dynamic_cast<IntLiteralExpr*>(add->left.get());
    ASSERT_NE(lhs, nullptr);
    EXPECT_EQ(lhs->value, 2);

    auto* mul = dynamic_cast<BinaryExpr*>(add->right.get());
    ASSERT_NE(mul, nullptr);
    EXPECT_EQ(mul->op, "*");
}

TEST(ParserTest, ParenthesizedExpr) {
    // (2 + 3) * 4 should parse as *(+(2, 3), 4)
    auto program = parseString("fn f() -> i32 { return (2 + 3) * 4; }");
    auto& stmts = program->functions[0]->body->statements;
    auto* ret = dynamic_cast<ReturnStmt*>(stmts[0].get());
    auto* mul = dynamic_cast<BinaryExpr*>(ret->value.get());
    ASSERT_NE(mul, nullptr);
    EXPECT_EQ(mul->op, "*");

    auto* add = dynamic_cast<BinaryExpr*>(mul->left.get());
    ASSERT_NE(add, nullptr);
    EXPECT_EQ(add->op, "+");
}

TEST(ParserTest, LetStatement) {
    auto program = parseString("fn f() { let x: i32 = 5; }");
    auto& stmts = program->functions[0]->body->statements;
    ASSERT_EQ(stmts.size(), 1u);
    auto* let = dynamic_cast<LetStmt*>(stmts[0].get());
    ASSERT_NE(let, nullptr);
    EXPECT_EQ(let->name, "x");
    EXPECT_EQ(let->type, SlangType::I32);
    EXPECT_FALSE(let->isMutable);
}

TEST(ParserTest, LetMutStatement) {
    auto program = parseString("fn f() { let mut y: f64 = 3.14; }");
    auto& stmts = program->functions[0]->body->statements;
    auto* let = dynamic_cast<LetStmt*>(stmts[0].get());
    ASSERT_NE(let, nullptr);
    EXPECT_EQ(let->name, "y");
    EXPECT_EQ(let->type, SlangType::F64);
    EXPECT_TRUE(let->isMutable);
}

TEST(ParserTest, AssignStatement) {
    auto program = parseString("fn f() { let mut x: i32 = 0; x = 10; }");
    auto& stmts = program->functions[0]->body->statements;
    ASSERT_EQ(stmts.size(), 2u);
    auto* assign = dynamic_cast<AssignStmt*>(stmts[1].get());
    ASSERT_NE(assign, nullptr);
    EXPECT_EQ(assign->name, "x");
}

TEST(ParserTest, ReturnStatement) {
    auto program = parseString("fn f() -> i32 { return 0; }");
    auto& stmts = program->functions[0]->body->statements;
    auto* ret = dynamic_cast<ReturnStmt*>(stmts[0].get());
    ASSERT_NE(ret, nullptr);
}

TEST(ParserTest, PrintStatement) {
    auto program = parseString("fn f() { print(42); }");
    auto& stmts = program->functions[0]->body->statements;
    auto* prt = dynamic_cast<PrintStmt*>(stmts[0].get());
    ASSERT_NE(prt, nullptr);
}

TEST(ParserTest, IfElseStatement) {
    auto program = parseString("fn f() { if true { return 1; } else { return 0; } }");
    auto& stmts = program->functions[0]->body->statements;
    auto* ifStmt = dynamic_cast<IfStmt*>(stmts[0].get());
    ASSERT_NE(ifStmt, nullptr);
    EXPECT_NE(ifStmt->thenBlock, nullptr);
    EXPECT_NE(ifStmt->elseBlock, nullptr);
}

TEST(ParserTest, WhileStatement) {
    auto program = parseString("fn f() { let mut i: i32 = 0; while i < 10 { i = i + 1; } }");
    auto& stmts = program->functions[0]->body->statements;
    ASSERT_EQ(stmts.size(), 2u);
    auto* whileStmt = dynamic_cast<WhileStmt*>(stmts[1].get());
    ASSERT_NE(whileStmt, nullptr);
}

TEST(ParserTest, FunctionWithParams) {
    auto program = parseString("fn add(a: i32, b: i32) -> i32 { return a + b; }");
    ASSERT_EQ(program->functions.size(), 1u);
    auto& fn = program->functions[0];
    EXPECT_EQ(fn->name, "add");
    EXPECT_EQ(fn->params.size(), 2u);
    EXPECT_EQ(fn->params[0].name, "a");
    EXPECT_EQ(fn->params[0].type, SlangType::I32);
    EXPECT_EQ(fn->params[1].name, "b");
    EXPECT_EQ(fn->returnType, SlangType::I32);
}

TEST(ParserTest, MultipleFunctions) {
    auto program = parseString(R"(
        fn helper() -> i32 { return 1; }
        fn main() -> i32 { return 0; }
    )");
    ASSERT_EQ(program->functions.size(), 2u);
    EXPECT_EQ(program->functions[0]->name, "helper");
    EXPECT_EQ(program->functions[1]->name, "main");
}

TEST(ParserTest, FunctionCall) {
    auto program = parseString("fn f() -> i32 { return add(1, 2); }");
    auto& stmts = program->functions[0]->body->statements;
    auto* ret = dynamic_cast<ReturnStmt*>(stmts[0].get());
    auto* call = dynamic_cast<CallExpr*>(ret->value.get());
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->callee, "add");
    EXPECT_EQ(call->args.size(), 2u);
}

TEST(ParserTest, UnaryExpression) {
    auto program = parseString("fn f() -> i32 { return -5; }");
    auto& stmts = program->functions[0]->body->statements;
    auto* ret = dynamic_cast<ReturnStmt*>(stmts[0].get());
    auto* unary = dynamic_cast<UnaryExpr*>(ret->value.get());
    ASSERT_NE(unary, nullptr);
    EXPECT_EQ(unary->op, "-");
}

TEST(ParserTest, ParseError) {
    EXPECT_THROW(parseString("fn { }"), ParseError);
}
