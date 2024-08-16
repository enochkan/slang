#include "lexer.h"

Lexer::Lexer(std::istream& input) : input(input) {
    advance();
}

void Lexer::advance() {
    currentChar = input.get();
}

Token Lexer::getNextToken() {
    while (isspace(currentChar)) {
        advance();
    }

    if (isdigit(currentChar)) {
        std::string value;
        while (isdigit(currentChar)) {
            value += currentChar;
            advance();
        }
        return {NUMBER, value};
    }

    if (isalpha(currentChar)) {
        std::string value;
        while (isalpha(currentChar)) {
            value += currentChar;
            advance();
        }
        return {IDENTIFIER, value};
    }

    if (currentChar == EOF) {
        return {END_OF_FILE, ""};
    }

    std::string value(1, currentChar);
    advance();
    return {OPERATOR, value};
}