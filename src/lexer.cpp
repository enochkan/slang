#include "lexer.h"
#include <cctype>

const std::unordered_map<std::string, TokenType> Lexer::keywords = {
    {"fn",     TokenType::KW_FN},
    {"let",    TokenType::KW_LET},
    {"mut",    TokenType::KW_MUT},
    {"return", TokenType::KW_RETURN},
    {"if",     TokenType::KW_IF},
    {"else",   TokenType::KW_ELSE},
    {"while",  TokenType::KW_WHILE},
    {"for",    TokenType::KW_FOR},
    {"in",     TokenType::KW_IN},
    {"print",  TokenType::KW_PRINT},
    {"true",   TokenType::KW_TRUE},
    {"false",  TokenType::KW_FALSE},
    {"i32",    TokenType::KW_I32},
    {"f64",    TokenType::KW_F64},
    {"bool",   TokenType::KW_BOOL},
};

Lexer::Lexer(std::istream& input)
    : input(input), currentChar('\0'), line(1), column(0), hasPeeked(false) {
    advance();
}

void Lexer::advance() {
    int ch = input.get();
    if (ch == '\n') {
        line++;
        column = 0;
    } else {
        column++;
    }
    currentChar = (ch == EOF) ? '\0' : static_cast<char>(ch);
}

char Lexer::peek() {
    int ch = input.peek();
    return (ch == EOF) ? '\0' : static_cast<char>(ch);
}

void Lexer::skipWhitespace() {
    while (currentChar != '\0' && std::isspace(currentChar)) {
        advance();
    }
}

void Lexer::skipLineComment() {
    while (currentChar != '\0' && currentChar != '\n') {
        advance();
    }
    if (currentChar == '\n') {
        advance();
    }
}

Token Lexer::readNumber() {
    int startCol = column;
    std::string value;

    while (currentChar != '\0' && std::isdigit(currentChar)) {
        value += currentChar;
        advance();
    }

    if (currentChar == '.' && std::isdigit(peek())) {
        value += currentChar;
        advance();
        while (currentChar != '\0' && std::isdigit(currentChar)) {
            value += currentChar;
            advance();
        }
        return Token(TokenType::FLOAT_LITERAL, value, line, startCol);
    }

    return Token(TokenType::INT_LITERAL, value, line, startCol);
}

Token Lexer::readIdentifierOrKeyword() {
    int startCol = column;
    std::string value;

    while (currentChar != '\0' && (std::isalnum(currentChar) || currentChar == '_')) {
        value += currentChar;
        advance();
    }

    // Check for boolean literals
    if (value == "true" || value == "false") {
        return Token(TokenType::BOOL_LITERAL, value, line, startCol);
    }

    // Check for keywords
    auto it = keywords.find(value);
    if (it != keywords.end()) {
        return Token(it->second, value, line, startCol);
    }

    return Token(TokenType::IDENTIFIER, value, line, startCol);
}

Token Lexer::peekToken() {
    if (!hasPeeked) {
        peekedToken = getNextToken();
        hasPeeked = true;
    }
    return peekedToken;
}

Token Lexer::getNextToken() {
    if (hasPeeked) {
        hasPeeked = false;
        return peekedToken;
    }

    skipWhitespace();

    // Check for comments
    while (currentChar == '/' && peek() == '/') {
        skipLineComment();
        skipWhitespace();
    }

    if (currentChar == '\0') {
        return Token(TokenType::END_OF_FILE, "", line, column);
    }

    int startCol = column;

    // Numbers
    if (std::isdigit(currentChar)) {
        return readNumber();
    }

    // Identifiers and keywords
    if (std::isalpha(currentChar) || currentChar == '_') {
        return readIdentifierOrKeyword();
    }

    // Multi-char and single-char operators/punctuation
    switch (currentChar) {
        case '+': advance(); return Token(TokenType::PLUS, "+", line, startCol);
        case '*': advance(); return Token(TokenType::STAR, "*", line, startCol);
        case '(': advance(); return Token(TokenType::LPAREN,    "(", line, startCol);
        case ')': advance(); return Token(TokenType::RPAREN,    ")", line, startCol);
        case '{': advance(); return Token(TokenType::LBRACE,    "{", line, startCol);
        case '}': advance(); return Token(TokenType::RBRACE,    "}", line, startCol);
        case '[': advance(); return Token(TokenType::LBRACKET,  "[", line, startCol);
        case ']': advance(); return Token(TokenType::RBRACKET,  "]", line, startCol);
        case ',': advance(); return Token(TokenType::COMMA,     ",", line, startCol);
        case ':': advance(); return Token(TokenType::COLON,     ":", line, startCol);
        case ';': advance(); return Token(TokenType::SEMICOLON, ";", line, startCol);

        case '.':
            advance();
            if (currentChar == '.') {
                advance();
                return Token(TokenType::DOTDOT, "..", line, startCol);
            }
            // Single '.' not currently supported
            break;

        case '-':
            advance();
            if (currentChar == '>') {
                advance();
                return Token(TokenType::ARROW, "->", line, startCol);
            }
            return Token(TokenType::MINUS, "-", line, startCol);

        case '/':
            advance();
            return Token(TokenType::SLASH, "/", line, startCol);

        case '=':
            advance();
            if (currentChar == '=') {
                advance();
                return Token(TokenType::EQ_EQ, "==", line, startCol);
            }
            return Token(TokenType::EQUALS, "=", line, startCol);

        case '!':
            advance();
            if (currentChar == '=') {
                advance();
                return Token(TokenType::BANG_EQ, "!=", line, startCol);
            }
            return Token(TokenType::BANG, "!", line, startCol);

        case '<':
            advance();
            if (currentChar == '=') {
                advance();
                return Token(TokenType::LESS_EQ, "<=", line, startCol);
            }
            return Token(TokenType::LESS, "<", line, startCol);

        case '>':
            advance();
            if (currentChar == '=') {
                advance();
                return Token(TokenType::GREATER_EQ, ">=", line, startCol);
            }
            return Token(TokenType::GREATER, ">", line, startCol);

        case '&':
            advance();
            if (currentChar == '&') {
                advance();
                return Token(TokenType::AMP_AMP, "&&", line, startCol);
            }
            // Single & not supported, treat as error - fall through
            break;

        case '|':
            advance();
            if (currentChar == '|') {
                advance();
                return Token(TokenType::PIPE_PIPE, "||", line, startCol);
            }
            // Single | not supported
            break;
    }

    // Unknown character - skip it
    std::string val(1, currentChar);
    advance();
    return Token(TokenType::END_OF_FILE, val, line, startCol);
}

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::INT_LITERAL:   return "INT_LITERAL";
        case TokenType::FLOAT_LITERAL: return "FLOAT_LITERAL";
        case TokenType::BOOL_LITERAL:  return "BOOL_LITERAL";
        case TokenType::IDENTIFIER:    return "IDENTIFIER";
        case TokenType::KW_FN:        return "KW_FN";
        case TokenType::KW_LET:       return "KW_LET";
        case TokenType::KW_MUT:       return "KW_MUT";
        case TokenType::KW_RETURN:    return "KW_RETURN";
        case TokenType::KW_IF:        return "KW_IF";
        case TokenType::KW_ELSE:      return "KW_ELSE";
        case TokenType::KW_WHILE:     return "KW_WHILE";
        case TokenType::KW_FOR:       return "KW_FOR";
        case TokenType::KW_IN:        return "KW_IN";
        case TokenType::KW_PRINT:     return "KW_PRINT";
        case TokenType::KW_TRUE:      return "KW_TRUE";
        case TokenType::KW_FALSE:     return "KW_FALSE";
        case TokenType::KW_I32:       return "KW_I32";
        case TokenType::KW_F64:       return "KW_F64";
        case TokenType::KW_BOOL:      return "KW_BOOL";
        case TokenType::PLUS:         return "PLUS";
        case TokenType::MINUS:        return "MINUS";
        case TokenType::STAR:         return "STAR";
        case TokenType::SLASH:        return "SLASH";
        case TokenType::EQ_EQ:        return "EQ_EQ";
        case TokenType::BANG_EQ:      return "BANG_EQ";
        case TokenType::LESS:         return "LESS";
        case TokenType::GREATER:      return "GREATER";
        case TokenType::LESS_EQ:      return "LESS_EQ";
        case TokenType::GREATER_EQ:   return "GREATER_EQ";
        case TokenType::AMP_AMP:      return "AMP_AMP";
        case TokenType::PIPE_PIPE:    return "PIPE_PIPE";
        case TokenType::BANG:         return "BANG";
        case TokenType::EQUALS:       return "EQUALS";
        case TokenType::LPAREN:       return "LPAREN";
        case TokenType::RPAREN:       return "RPAREN";
        case TokenType::LBRACE:       return "LBRACE";
        case TokenType::RBRACE:       return "RBRACE";
        case TokenType::LBRACKET:     return "LBRACKET";
        case TokenType::RBRACKET:     return "RBRACKET";
        case TokenType::DOTDOT:       return "DOTDOT";
        case TokenType::COMMA:        return "COMMA";
        case TokenType::COLON:        return "COLON";
        case TokenType::SEMICOLON:    return "SEMICOLON";
        case TokenType::ARROW:        return "ARROW";
        case TokenType::END_OF_FILE:  return "END_OF_FILE";
        default:                       return "UNKNOWN";
    }
}
