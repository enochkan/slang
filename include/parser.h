#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"
#include <stdexcept>
#include <memory>

class ParseError : public std::runtime_error {
public:
    int line;
    int column;
    ParseError(const std::string& message, int line, int column)
        : std::runtime_error(message), line(line), column(column) {}
};

class Parser {
public:
    Parser(Lexer& lexer);
    std::unique_ptr<Program> parseProgram();

private:
    Lexer& lexer;
    Token currentToken;

    // Helpers
    void advance();
    Token expect(TokenType type, const std::string& message);
    bool check(TokenType type);
    bool match(TokenType type);
    SlangType parseType();

    // Top-level
    std::unique_ptr<FnDecl> parseFnDecl();

    // Statements
    std::unique_ptr<StmtNode> parseStatement();
    std::unique_ptr<LetStmt> parseLetStmt();
    std::unique_ptr<IfStmt> parseIfStmt();
    std::unique_ptr<WhileStmt> parseWhileStmt();
    std::unique_ptr<PrintStmt> parsePrintStmt();
    std::unique_ptr<ReturnStmt> parseReturnStmt();
    std::unique_ptr<ForStmt> parseForStmt();
    std::unique_ptr<StmtNode> parseAssignOrExprStmt();
    std::unique_ptr<BlockStmt> parseBlock();

    // Expressions (precedence climbing)
    std::unique_ptr<ExprNode> parseExpression();
    std::unique_ptr<ExprNode> parseLogicalOr();
    std::unique_ptr<ExprNode> parseLogicalAnd();
    std::unique_ptr<ExprNode> parseComparison();
    std::unique_ptr<ExprNode> parseAdditive();
    std::unique_ptr<ExprNode> parseMultiplicative();
    std::unique_ptr<ExprNode> parseUnary();
    std::unique_ptr<ExprNode> parsePrimary();
};

#endif
