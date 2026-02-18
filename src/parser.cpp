#include "parser.h"
#include <sstream>

Parser::Parser(Lexer& lexer) : lexer(lexer) {
    advance();
}

void Parser::advance() {
    currentToken = lexer.getNextToken();
}

Token Parser::expect(TokenType type, const std::string& message) {
    if (currentToken.type != type) {
        std::ostringstream oss;
        oss << message << " at line " << currentToken.line << ":" << currentToken.column
            << ", got '" << currentToken.value << "' (" << tokenTypeToString(currentToken.type) << ")";
        throw ParseError(oss.str(), currentToken.line, currentToken.column);
    }
    Token tok = currentToken;
    advance();
    return tok;
}

bool Parser::check(TokenType type) {
    return currentToken.type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

SlangType Parser::parseType() {
    if (match(TokenType::KW_I32))  return SlangType::I32;
    if (match(TokenType::KW_F64))  return SlangType::F64;
    if (match(TokenType::KW_BOOL)) return SlangType::Bool;

    std::ostringstream oss;
    oss << "Expected type (i32, f64, bool) at line " << currentToken.line << ":" << currentToken.column;
    throw ParseError(oss.str(), currentToken.line, currentToken.column);
}

// --- Top-level ---

std::unique_ptr<Program> Parser::parseProgram() {
    auto program = std::make_unique<Program>();
    while (!check(TokenType::END_OF_FILE)) {
        program->functions.push_back(parseFnDecl());
    }
    return program;
}

std::unique_ptr<FnDecl> Parser::parseFnDecl() {
    expect(TokenType::KW_FN, "Expected 'fn'");
    Token name = expect(TokenType::IDENTIFIER, "Expected function name");

    expect(TokenType::LPAREN, "Expected '(' after function name");
    std::vector<FnParam> params;
    if (!check(TokenType::RPAREN)) {
        do {
            Token paramName = expect(TokenType::IDENTIFIER, "Expected parameter name");
            expect(TokenType::COLON, "Expected ':' after parameter name");
            SlangType paramType = parseType();
            params.emplace_back(paramName.value, paramType);
        } while (match(TokenType::COMMA));
    }
    expect(TokenType::RPAREN, "Expected ')' after parameters");

    SlangType returnType = SlangType::Void;
    if (match(TokenType::ARROW)) {
        returnType = parseType();
    }

    auto body = parseBlock();
    return std::make_unique<FnDecl>(name.value, std::move(params), returnType, std::move(body));
}

// --- Statements ---

std::unique_ptr<BlockStmt> Parser::parseBlock() {
    expect(TokenType::LBRACE, "Expected '{'");
    std::vector<std::unique_ptr<StmtNode>> statements;
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        statements.push_back(parseStatement());
    }
    expect(TokenType::RBRACE, "Expected '}'");
    return std::make_unique<BlockStmt>(std::move(statements));
}

std::unique_ptr<StmtNode> Parser::parseStatement() {
    if (check(TokenType::KW_LET))    return parseLetStmt();
    if (check(TokenType::KW_IF))     return parseIfStmt();
    if (check(TokenType::KW_WHILE))  return parseWhileStmt();
    if (check(TokenType::KW_FOR))    return parseForStmt();
    if (check(TokenType::KW_PRINT))  return parsePrintStmt();
    if (check(TokenType::KW_RETURN)) return parseReturnStmt();
    return parseAssignOrExprStmt();
}

std::unique_ptr<LetStmt> Parser::parseLetStmt() {
    expect(TokenType::KW_LET, "Expected 'let'");
    bool isMutable = match(TokenType::KW_MUT);
    Token name = expect(TokenType::IDENTIFIER, "Expected variable name");
    expect(TokenType::COLON, "Expected ':' after variable name");

    // Array type annotation: [elemType; N]
    if (check(TokenType::LBRACKET)) {
        advance(); // consume '['
        SlangType elemType = parseType();
        expect(TokenType::SEMICOLON, "Expected ';' in array type (e.g. [i32; 5])");
        Token sizeTok = expect(TokenType::INT_LITERAL, "Expected array size");
        int arraySize = std::stoi(sizeTok.value);
        expect(TokenType::RBRACKET, "Expected ']' after array type");
        expect(TokenType::EQUALS, "Expected '=' in let statement");
        auto initializer = parseExpression();
        expect(TokenType::SEMICOLON, "Expected ';' after let statement");
        return std::make_unique<LetStmt>(name.value, elemType, arraySize, isMutable, std::move(initializer));
    }

    SlangType type = parseType();
    expect(TokenType::EQUALS, "Expected '=' in let statement");
    auto initializer = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';' after let statement");
    return std::make_unique<LetStmt>(name.value, type, isMutable, std::move(initializer));
}

std::unique_ptr<ForStmt> Parser::parseForStmt() {
    expect(TokenType::KW_FOR, "Expected 'for'");
    Token var = expect(TokenType::IDENTIFIER, "Expected loop variable name");
    expect(TokenType::KW_IN, "Expected 'in' after loop variable");
    auto start = parseExpression(); // stops naturally before '..' (not an operator)
    expect(TokenType::DOTDOT, "Expected '..' in range expression");
    auto end = parseExpression();
    auto body = parseBlock();
    return std::make_unique<ForStmt>(var.value, std::move(start), std::move(end), std::move(body));
}

std::unique_ptr<IfStmt> Parser::parseIfStmt() {
    expect(TokenType::KW_IF, "Expected 'if'");
    auto condition = parseExpression();
    auto thenBlock = parseBlock();
    std::unique_ptr<BlockStmt> elseBlock = nullptr;
    if (match(TokenType::KW_ELSE)) {
        elseBlock = parseBlock();
    }
    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBlock), std::move(elseBlock));
}

std::unique_ptr<WhileStmt> Parser::parseWhileStmt() {
    expect(TokenType::KW_WHILE, "Expected 'while'");
    auto condition = parseExpression();
    auto body = parseBlock();
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<PrintStmt> Parser::parsePrintStmt() {
    expect(TokenType::KW_PRINT, "Expected 'print'");
    expect(TokenType::LPAREN, "Expected '(' after print");
    auto expr = parseExpression();
    expect(TokenType::RPAREN, "Expected ')' after print argument");
    expect(TokenType::SEMICOLON, "Expected ';' after print statement");
    return std::make_unique<PrintStmt>(std::move(expr));
}

std::unique_ptr<ReturnStmt> Parser::parseReturnStmt() {
    expect(TokenType::KW_RETURN, "Expected 'return'");
    auto value = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';' after return statement");
    return std::make_unique<ReturnStmt>(std::move(value));
}

std::unique_ptr<StmtNode> Parser::parseAssignOrExprStmt() {
    if (check(TokenType::IDENTIFIER)) {
        Token peeked = lexer.peekToken();

        // Scalar assignment: name = expr;
        if (peeked.type == TokenType::EQUALS) {
            Token name = currentToken;
            advance(); // consume identifier
            advance(); // consume '='
            auto value = parseExpression();
            expect(TokenType::SEMICOLON, "Expected ';' after assignment");
            return std::make_unique<AssignStmt>(name.value, std::move(value));
        }

        // Array index assignment: name[index] = expr;
        if (peeked.type == TokenType::LBRACKET) {
            std::string name = currentToken.value;
            advance(); // consume identifier
            advance(); // consume '['
            auto index = parseExpression();
            expect(TokenType::RBRACKET, "Expected ']' after index");
            expect(TokenType::EQUALS, "Expected '=' in array assignment");
            auto value = parseExpression();
            expect(TokenType::SEMICOLON, "Expected ';' after array assignment");
            return std::make_unique<ArrayAssignStmt>(name, std::move(index), std::move(value));
        }
    }

    // Otherwise it's an expression statement
    auto expr = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';' after expression statement");
    return std::make_unique<ExprStmt>(std::move(expr));
}

// --- Expressions ---

std::unique_ptr<ExprNode> Parser::parseExpression() {
    return parseLogicalOr();
}

std::unique_ptr<ExprNode> Parser::parseLogicalOr() {
    auto left = parseLogicalAnd();
    while (check(TokenType::PIPE_PIPE)) {
        std::string op = currentToken.value;
        advance();
        auto right = parseLogicalAnd();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<ExprNode> Parser::parseLogicalAnd() {
    auto left = parseComparison();
    while (check(TokenType::AMP_AMP)) {
        std::string op = currentToken.value;
        advance();
        auto right = parseComparison();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<ExprNode> Parser::parseComparison() {
    auto left = parseAdditive();
    while (check(TokenType::EQ_EQ) || check(TokenType::BANG_EQ) ||
           check(TokenType::LESS) || check(TokenType::GREATER) ||
           check(TokenType::LESS_EQ) || check(TokenType::GREATER_EQ)) {
        std::string op = currentToken.value;
        advance();
        auto right = parseAdditive();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<ExprNode> Parser::parseAdditive() {
    auto left = parseMultiplicative();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        std::string op = currentToken.value;
        advance();
        auto right = parseMultiplicative();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<ExprNode> Parser::parseMultiplicative() {
    auto left = parseUnary();
    while (check(TokenType::STAR) || check(TokenType::SLASH)) {
        std::string op = currentToken.value;
        advance();
        auto right = parseUnary();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<ExprNode> Parser::parseUnary() {
    if (check(TokenType::MINUS) || check(TokenType::BANG)) {
        std::string op = currentToken.value;
        advance();
        auto operand = parseUnary();
        return std::make_unique<UnaryExpr>(op, std::move(operand));
    }
    return parsePrimary();
}

std::unique_ptr<ExprNode> Parser::parsePrimary() {
    // Integer literal
    if (check(TokenType::INT_LITERAL)) {
        int val = std::stoi(currentToken.value);
        advance();
        return std::make_unique<IntLiteralExpr>(val);
    }

    // Float literal
    if (check(TokenType::FLOAT_LITERAL)) {
        double val = std::stod(currentToken.value);
        advance();
        return std::make_unique<FloatLiteralExpr>(val);
    }

    // Bool literal
    if (check(TokenType::BOOL_LITERAL)) {
        bool val = (currentToken.value == "true");
        advance();
        return std::make_unique<BoolLiteralExpr>(val);
    }

    // Array literal: [expr, expr, ...]
    if (check(TokenType::LBRACKET)) {
        advance(); // consume '['
        std::vector<std::unique_ptr<ExprNode>> elements;
        if (!check(TokenType::RBRACKET)) {
            elements.push_back(parseExpression());
            while (match(TokenType::COMMA)) {
                elements.push_back(parseExpression());
            }
        }
        expect(TokenType::RBRACKET, "Expected ']' after array elements");
        return std::make_unique<ArrayLiteralExpr>(std::move(elements));
    }

    // Identifier, function call, or array index read
    if (check(TokenType::IDENTIFIER)) {
        std::string name = currentToken.value;
        advance();

        // Array index read: name[expr]
        if (check(TokenType::LBRACKET)) {
            advance(); // consume '['
            auto index = parseExpression();
            expect(TokenType::RBRACKET, "Expected ']' after index");
            return std::make_unique<ArrayIndexExpr>(name, std::move(index));
        }

        // Function call: name(args)
        if (check(TokenType::LPAREN)) {
            advance(); // consume '('
            std::vector<std::unique_ptr<ExprNode>> args;
            if (!check(TokenType::RPAREN)) {
                args.push_back(parseExpression());
                while (match(TokenType::COMMA)) {
                    args.push_back(parseExpression());
                }
            }
            expect(TokenType::RPAREN, "Expected ')' after function arguments");
            return std::make_unique<CallExpr>(name, std::move(args));
        }

        return std::make_unique<VariableExpr>(name);
    }

    // Grouped expression: (expr)
    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after grouped expression");
        return expr;
    }

    std::ostringstream oss;
    oss << "Unexpected token '" << currentToken.value << "' at line "
        << currentToken.line << ":" << currentToken.column;
    throw ParseError(oss.str(), currentToken.line, currentToken.column);
}
