#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <unordered_map>
#include <iostream>

enum class TokenType {
    // Literals
    INT_LITERAL,
    FLOAT_LITERAL,
    BOOL_LITERAL,
    IDENTIFIER,

    // Keywords
    KW_FN,
    KW_LET,
    KW_MUT,
    KW_RETURN,
    KW_IF,
    KW_ELSE,
    KW_WHILE,
    KW_PRINT,
    KW_TRUE,
    KW_FALSE,

    // Type keywords
    KW_I32,
    KW_F64,
    KW_BOOL,

    // Arithmetic operators
    PLUS,
    MINUS,
    STAR,
    SLASH,

    // Comparison operators
    EQ_EQ,
    BANG_EQ,
    LESS,
    GREATER,
    LESS_EQ,
    GREATER_EQ,

    // Logical operators
    AMP_AMP,
    PIPE_PIPE,
    BANG,

    // Assignment
    EQUALS,

    // Punctuation
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    COMMA,
    COLON,
    SEMICOLON,

    // Arrow
    ARROW,

    // End of file
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;

    Token() : type(TokenType::END_OF_FILE), line(0), column(0) {}
    Token(TokenType type, const std::string& value, int line, int column)
        : type(type), value(value), line(line), column(column) {}
};

class Lexer {
public:
    Lexer(std::istream& input);
    Token getNextToken();
    Token peekToken();

private:
    std::istream& input;
    char currentChar;
    int line;
    int column;
    bool hasPeeked;
    Token peekedToken;

    static const std::unordered_map<std::string, TokenType> keywords;

    void advance();
    char peek();
    void skipWhitespace();
    void skipLineComment();
    Token readNumber();
    Token readIdentifierOrKeyword();
};

std::string tokenTypeToString(TokenType type);

#endif
