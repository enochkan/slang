#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <iostream>

enum TokenType { IDENTIFIER, NUMBER, OPERATOR, KEYWORD, END_OF_FILE };

struct Token {
    TokenType type;
    std::string value;
};

class Lexer {
public:
    Lexer(std::istream& input);
    Token getNextToken();

private:
    std::istream& input;
    char currentChar;
    void advance();
};

#endif
