#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>

enum class SlangType {
    I32,
    F64,
    Bool,
    Void
};

// --- Expression Nodes ---

struct ExprNode {
    virtual ~ExprNode() = default;
};

struct IntLiteralExpr : ExprNode {
    int value;
    IntLiteralExpr(int value) : value(value) {}
};

struct FloatLiteralExpr : ExprNode {
    double value;
    FloatLiteralExpr(double value) : value(value) {}
};

struct BoolLiteralExpr : ExprNode {
    bool value;
    BoolLiteralExpr(bool value) : value(value) {}
};

struct VariableExpr : ExprNode {
    std::string name;
    VariableExpr(const std::string& name) : name(name) {}
};

struct BinaryExpr : ExprNode {
    std::string op;
    std::unique_ptr<ExprNode> left;
    std::unique_ptr<ExprNode> right;
    BinaryExpr(const std::string& op, std::unique_ptr<ExprNode> left, std::unique_ptr<ExprNode> right)
        : op(op), left(std::move(left)), right(std::move(right)) {}
};

struct UnaryExpr : ExprNode {
    std::string op;
    std::unique_ptr<ExprNode> operand;
    UnaryExpr(const std::string& op, std::unique_ptr<ExprNode> operand)
        : op(op), operand(std::move(operand)) {}
};

struct CallExpr : ExprNode {
    std::string callee;
    std::vector<std::unique_ptr<ExprNode>> args;
    CallExpr(const std::string& callee, std::vector<std::unique_ptr<ExprNode>> args)
        : callee(callee), args(std::move(args)) {}
};

// --- Statement Nodes ---

struct StmtNode {
    virtual ~StmtNode() = default;
};

struct LetStmt : StmtNode {
    std::string name;
    SlangType type;
    bool isMutable;
    std::unique_ptr<ExprNode> initializer;
    LetStmt(const std::string& name, SlangType type, bool isMutable, std::unique_ptr<ExprNode> initializer)
        : name(name), type(type), isMutable(isMutable), initializer(std::move(initializer)) {}
};

struct AssignStmt : StmtNode {
    std::string name;
    std::unique_ptr<ExprNode> value;
    AssignStmt(const std::string& name, std::unique_ptr<ExprNode> value)
        : name(name), value(std::move(value)) {}
};

struct ReturnStmt : StmtNode {
    std::unique_ptr<ExprNode> value;
    ReturnStmt(std::unique_ptr<ExprNode> value) : value(std::move(value)) {}
};

struct ExprStmt : StmtNode {
    std::unique_ptr<ExprNode> expr;
    ExprStmt(std::unique_ptr<ExprNode> expr) : expr(std::move(expr)) {}
};

struct PrintStmt : StmtNode {
    std::unique_ptr<ExprNode> expr;
    PrintStmt(std::unique_ptr<ExprNode> expr) : expr(std::move(expr)) {}
};

struct BlockStmt : StmtNode {
    std::vector<std::unique_ptr<StmtNode>> statements;
    BlockStmt(std::vector<std::unique_ptr<StmtNode>> statements)
        : statements(std::move(statements)) {}
};

struct IfStmt : StmtNode {
    std::unique_ptr<ExprNode> condition;
    std::unique_ptr<BlockStmt> thenBlock;
    std::unique_ptr<BlockStmt> elseBlock; // nullable
    IfStmt(std::unique_ptr<ExprNode> condition,
           std::unique_ptr<BlockStmt> thenBlock,
           std::unique_ptr<BlockStmt> elseBlock)
        : condition(std::move(condition)),
          thenBlock(std::move(thenBlock)),
          elseBlock(std::move(elseBlock)) {}
};

struct WhileStmt : StmtNode {
    std::unique_ptr<ExprNode> condition;
    std::unique_ptr<BlockStmt> body;
    WhileStmt(std::unique_ptr<ExprNode> condition, std::unique_ptr<BlockStmt> body)
        : condition(std::move(condition)), body(std::move(body)) {}
};

// --- Top-level ---

struct FnParam {
    std::string name;
    SlangType type;
    FnParam(const std::string& name, SlangType type) : name(name), type(type) {}
};

struct FnDecl {
    std::string name;
    std::vector<FnParam> params;
    SlangType returnType;
    std::unique_ptr<BlockStmt> body;
    FnDecl(const std::string& name, std::vector<FnParam> params, SlangType returnType,
           std::unique_ptr<BlockStmt> body)
        : name(name), params(std::move(params)), returnType(returnType), body(std::move(body)) {}
};

struct Program {
    std::vector<std::unique_ptr<FnDecl>> functions;
};

#endif
