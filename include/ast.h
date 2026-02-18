#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>

enum class SlangType {
    I32,
    F64,
    Bool,
    Void,
    Array,
    Struct
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

// Array literal: [1, 2, 3]
struct ArrayLiteralExpr : ExprNode {
    std::vector<std::unique_ptr<ExprNode>> elements;
    ArrayLiteralExpr(std::vector<std::unique_ptr<ExprNode>> elements)
        : elements(std::move(elements)) {}
};

// Struct initialization: Point { x: 1.0, y: 2.0 }
// Only valid in let statement initializer position.
struct StructInitExpr : ExprNode {
    std::string structName;
    std::vector<std::pair<std::string, std::unique_ptr<ExprNode>>> fields;
    StructInitExpr(const std::string& sn,
                   std::vector<std::pair<std::string, std::unique_ptr<ExprNode>>> f)
        : structName(sn), fields(std::move(f)) {}
};

// Field access — works for both scalar structs (p.x) and SoA arrays (points[i].x).
// arrayIndex is nullptr for scalar struct access.
struct FieldAccessExpr : ExprNode {
    std::string varName;
    std::unique_ptr<ExprNode> arrayIndex;
    std::string field;
    FieldAccessExpr(const std::string& vn, std::unique_ptr<ExprNode> idx, const std::string& f)
        : varName(vn), arrayIndex(std::move(idx)), field(f) {}
};

// Array index read: arr[i]
struct ArrayIndexExpr : ExprNode {
    std::string name;
    std::unique_ptr<ExprNode> index;
    ArrayIndexExpr(const std::string& name, std::unique_ptr<ExprNode> index)
        : name(name), index(std::move(index)) {}
};

// --- Statement Nodes ---

struct StmtNode {
    virtual ~StmtNode() = default;
};

struct LetStmt : StmtNode {
    std::string name;
    SlangType type;
    bool isMutable;
    std::unique_ptr<ExprNode> initializer; // may be nullptr for struct arrays
    // Array fields (type == Array)
    SlangType elemType = SlangType::I32;
    int arraySize = -1;
    // Struct name (type == Struct, or type == Array && elemType == Struct)
    std::string structName;

    // Scalar constructor (i32, f64, bool)
    LetStmt(const std::string& name, SlangType type, bool isMutable, std::unique_ptr<ExprNode> init)
        : name(name), type(type), isMutable(isMutable), initializer(std::move(init)) {}

    // Scalar-array constructor: let arr: [i32; N] = [...]
    LetStmt(const std::string& name, SlangType elemType, int arraySize, bool isMutable,
            std::unique_ptr<ExprNode> init)
        : name(name), type(SlangType::Array), isMutable(isMutable), initializer(std::move(init)),
          elemType(elemType), arraySize(arraySize) {}

    // Struct constructor: let p: Point = Point { ... }
    LetStmt(const std::string& name, const std::string& sName, bool isMutable,
            std::unique_ptr<ExprNode> init)
        : name(name), type(SlangType::Struct), isMutable(isMutable), initializer(std::move(init)),
          structName(sName) {}

    // Struct-array constructor: let mut ps: [Point; N]  (init may be nullptr → zero init)
    LetStmt(const std::string& name, const std::string& sName, int arraySize, bool isMutable,
            std::unique_ptr<ExprNode> init)
        : name(name), type(SlangType::Array), isMutable(isMutable), initializer(std::move(init)),
          elemType(SlangType::Struct), arraySize(arraySize), structName(sName) {}
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

// Field assignment — works for scalar structs (p.x = val) and SoA arrays (points[i].x = val).
// arrayIndex is nullptr for scalar struct assignment.
struct FieldAssignStmt : StmtNode {
    std::string varName;
    std::unique_ptr<ExprNode> arrayIndex;
    std::string field;
    std::unique_ptr<ExprNode> value;
    FieldAssignStmt(const std::string& vn, std::unique_ptr<ExprNode> idx,
                    const std::string& f, std::unique_ptr<ExprNode> val)
        : varName(vn), arrayIndex(std::move(idx)), field(f), value(std::move(val)) {}
};

// Array index write: arr[i] = val
struct ArrayAssignStmt : StmtNode {
    std::string name;
    std::unique_ptr<ExprNode> index;
    std::unique_ptr<ExprNode> value;
    ArrayAssignStmt(const std::string& name, std::unique_ptr<ExprNode> index, std::unique_ptr<ExprNode> value)
        : name(name), index(std::move(index)), value(std::move(value)) {}
};

// For loop: for i in start..end { body }
struct ForStmt : StmtNode {
    std::string varName;
    std::unique_ptr<ExprNode> start;
    std::unique_ptr<ExprNode> end;
    std::unique_ptr<BlockStmt> body;
    ForStmt(const std::string& varName, std::unique_ptr<ExprNode> start,
            std::unique_ptr<ExprNode> end, std::unique_ptr<BlockStmt> body)
        : varName(varName), start(std::move(start)), end(std::move(end)), body(std::move(body)) {}
};

// --- Top-level ---

struct StructField {
    std::string name;
    SlangType type;
    StructField(const std::string& name, SlangType type) : name(name), type(type) {}
};

struct StructDecl {
    std::string name;
    std::vector<StructField> fields;
    StructDecl(const std::string& name, std::vector<StructField> fields)
        : name(name), fields(std::move(fields)) {}
};

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
    std::vector<std::unique_ptr<StructDecl>> structs;
    std::vector<std::unique_ptr<FnDecl>> functions;
};

#endif
