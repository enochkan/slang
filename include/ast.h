#ifndef AST_H
#define AST_H

#include <string>
#include <vector>

enum class ASTNodeType {
    Literal,
    BinaryOp,
    Variable,  // Ensure this exists
    // Add other node types as needed
};

struct ASTNode {
    ASTNodeType type;
    std::string value;
    std::vector<ASTNode*> children;

    // Declaration only, no implementation here
    ASTNode(ASTNodeType type, const std::string& value);
};

#endif