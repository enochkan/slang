#ifndef AST_H
#define AST_H

#include <string>
#include <vector>

enum ASTNodeType { Literal, BinaryOp };

struct ASTNode {
    ASTNodeType type;
    std::string value;
    std::vector<ASTNode*> children;

    ASTNode(ASTNodeType type, const std::string& value) : type(type), value(value) {}
};

#endif