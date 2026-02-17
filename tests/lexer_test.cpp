#include <gtest/gtest.h>
#include <sstream>
#include "lexer.h"

TEST(LexerTest, IntegerLiteral) {
    std::istringstream input("42");
    Lexer lexer(input);
    Token tok = lexer.getNextToken();
    EXPECT_EQ(tok.type, TokenType::INT_LITERAL);
    EXPECT_EQ(tok.value, "42");
}

TEST(LexerTest, FloatLiteral) {
    std::istringstream input("3.14");
    Lexer lexer(input);
    Token tok = lexer.getNextToken();
    EXPECT_EQ(tok.type, TokenType::FLOAT_LITERAL);
    EXPECT_EQ(tok.value, "3.14");
}

TEST(LexerTest, BoolLiterals) {
    std::istringstream input("true false");
    Lexer lexer(input);
    Token t1 = lexer.getNextToken();
    EXPECT_EQ(t1.type, TokenType::BOOL_LITERAL);
    EXPECT_EQ(t1.value, "true");

    Token t2 = lexer.getNextToken();
    EXPECT_EQ(t2.type, TokenType::BOOL_LITERAL);
    EXPECT_EQ(t2.value, "false");
}

TEST(LexerTest, Keywords) {
    std::istringstream input("fn let mut return if else while print i32 f64 bool");
    Lexer lexer(input);

    EXPECT_EQ(lexer.getNextToken().type, TokenType::KW_FN);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::KW_LET);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::KW_MUT);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::KW_RETURN);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::KW_IF);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::KW_ELSE);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::KW_WHILE);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::KW_PRINT);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::KW_I32);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::KW_F64);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::KW_BOOL);
}

TEST(LexerTest, Identifiers) {
    std::istringstream input("foo bar_baz x1");
    Lexer lexer(input);

    Token t1 = lexer.getNextToken();
    EXPECT_EQ(t1.type, TokenType::IDENTIFIER);
    EXPECT_EQ(t1.value, "foo");

    Token t2 = lexer.getNextToken();
    EXPECT_EQ(t2.type, TokenType::IDENTIFIER);
    EXPECT_EQ(t2.value, "bar_baz");

    Token t3 = lexer.getNextToken();
    EXPECT_EQ(t3.type, TokenType::IDENTIFIER);
    EXPECT_EQ(t3.value, "x1");
}

TEST(LexerTest, IdentifierNotKeyword) {
    std::istringstream input("letter letting fns");
    Lexer lexer(input);

    Token t1 = lexer.getNextToken();
    EXPECT_EQ(t1.type, TokenType::IDENTIFIER);
    EXPECT_EQ(t1.value, "letter");

    Token t2 = lexer.getNextToken();
    EXPECT_EQ(t2.type, TokenType::IDENTIFIER);
    EXPECT_EQ(t2.value, "letting");

    Token t3 = lexer.getNextToken();
    EXPECT_EQ(t3.type, TokenType::IDENTIFIER);
    EXPECT_EQ(t3.value, "fns");
}

TEST(LexerTest, MultiCharOperators) {
    std::istringstream input("== != <= >= && || ->");
    Lexer lexer(input);

    EXPECT_EQ(lexer.getNextToken().type, TokenType::EQ_EQ);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::BANG_EQ);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::LESS_EQ);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::GREATER_EQ);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::AMP_AMP);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::PIPE_PIPE);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::ARROW);
}

TEST(LexerTest, SingleCharTokens) {
    std::istringstream input("+ - * / = < > ! ( ) { } , : ;");
    Lexer lexer(input);

    EXPECT_EQ(lexer.getNextToken().type, TokenType::PLUS);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::MINUS);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::STAR);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::SLASH);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::EQUALS);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::LESS);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::GREATER);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::BANG);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::LPAREN);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::RPAREN);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::LBRACE);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::RBRACE);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::COMMA);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::COLON);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::SEMICOLON);
}

TEST(LexerTest, LetStatement) {
    std::istringstream input("let mut x: i32 = 5;");
    Lexer lexer(input);

    EXPECT_EQ(lexer.getNextToken().type, TokenType::KW_LET);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::KW_MUT);
    Token name = lexer.getNextToken();
    EXPECT_EQ(name.type, TokenType::IDENTIFIER);
    EXPECT_EQ(name.value, "x");
    EXPECT_EQ(lexer.getNextToken().type, TokenType::COLON);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::KW_I32);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::EQUALS);
    Token val = lexer.getNextToken();
    EXPECT_EQ(val.type, TokenType::INT_LITERAL);
    EXPECT_EQ(val.value, "5");
    EXPECT_EQ(lexer.getNextToken().type, TokenType::SEMICOLON);
    EXPECT_EQ(lexer.getNextToken().type, TokenType::END_OF_FILE);
}

TEST(LexerTest, CommentSkipping) {
    std::istringstream input("42 // this is a comment\n7");
    Lexer lexer(input);

    Token t1 = lexer.getNextToken();
    EXPECT_EQ(t1.type, TokenType::INT_LITERAL);
    EXPECT_EQ(t1.value, "42");

    Token t2 = lexer.getNextToken();
    EXPECT_EQ(t2.type, TokenType::INT_LITERAL);
    EXPECT_EQ(t2.value, "7");
}

TEST(LexerTest, LineColumnTracking) {
    std::istringstream input("fn main");
    Lexer lexer(input);

    Token t1 = lexer.getNextToken();
    EXPECT_EQ(t1.line, 1);
    EXPECT_EQ(t1.column, 1);

    Token t2 = lexer.getNextToken();
    EXPECT_EQ(t2.line, 1);
    EXPECT_EQ(t2.column, 4);
}

TEST(LexerTest, PeekToken) {
    std::istringstream input("1 + 2");
    Lexer lexer(input);

    Token peeked = lexer.peekToken();
    EXPECT_EQ(peeked.type, TokenType::INT_LITERAL);
    EXPECT_EQ(peeked.value, "1");

    // Peeking again returns the same token
    Token peeked2 = lexer.peekToken();
    EXPECT_EQ(peeked2.type, TokenType::INT_LITERAL);

    // Getting the next token returns the peeked token
    Token next = lexer.getNextToken();
    EXPECT_EQ(next.type, TokenType::INT_LITERAL);
    EXPECT_EQ(next.value, "1");

    // Next token is now +
    Token plus = lexer.getNextToken();
    EXPECT_EQ(plus.type, TokenType::PLUS);
}
