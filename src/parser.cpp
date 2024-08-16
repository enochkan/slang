#include "parser.h"

Parser::Parser(Lexer& lexer) : lexer(lexer) {
    advance();
}

void Parser::advance() {
    currentToken = lexer.getNextToken();
}

ASTNode* Parser::parse() {
    return parseExpression();  // Simple example: start with expressions
}

ASTNode* Parser::parseExpression() {
    // Build the AST based on tokens, for example:
    if (currentToken.type == NUMBER) {
        ASTNode* node = new ASTNode(ASTNodeType::Literal, currentToken.value);
        advance();
        return node;
    }
    // Handle more complex expressions...
}