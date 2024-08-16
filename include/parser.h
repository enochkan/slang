#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

class Parser {
public:
    Parser(Lexer& lexer);
    ASTNode* parse();

private:
    Lexer& lexer;
    Token currentToken;

    ASTNode* parseExpression();
    void advance();
};

#endif