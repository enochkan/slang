#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <unordered_map>
#include <memory>
#include <string>

namespace llvm { class TargetMachine; }

struct VarInfo {
    llvm::AllocaInst* alloca;
    SlangType type;
    bool isMutable;
    SlangType elemType = SlangType::I32; // only used when type == Array
    int arraySize = -1;                  // only used when type == Array
};

class CodeGenerator {
public:
    CodeGenerator();
    void generate(Program& program, const std::string& outputBaseName);

private:
    llvm::LLVMContext context;
    std::unique_ptr<llvm::Module> module;
    llvm::IRBuilder<> builder;
    std::unordered_map<std::string, VarInfo> namedValues;

    llvm::FunctionCallee printfFunc;

    // Helpers
    void declarePrintf();
    void emitIR(const std::string& filename);
    void emitObjectFile(const std::string& filename, llvm::TargetMachine* TM);
    void runOptimizationPasses(llvm::TargetMachine* TM);
    llvm::Type* getLLVMType(SlangType type);
    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* fn, const std::string& name, llvm::Type* type);
    SlangType inferExprType(ExprNode* expr);

    // Code generation for each node type
    void generateFnDecl(FnDecl& fn);

    // Statements
    void generateStmt(StmtNode* stmt);
    void generateLetStmt(LetStmt* stmt);
    void generateAssignStmt(AssignStmt* stmt);
    void generateReturnStmt(ReturnStmt* stmt);
    void generateExprStmt(ExprStmt* stmt);
    void generatePrintStmt(PrintStmt* stmt);
    void generateBlockStmt(BlockStmt* stmt);
    void generateIfStmt(IfStmt* stmt);
    void generateWhileStmt(WhileStmt* stmt);
    void generateForStmt(ForStmt* stmt);
    void generateArrayAssignStmt(ArrayAssignStmt* stmt);

    // Expressions
    llvm::Value* generateExpr(ExprNode* expr);
    llvm::Value* generateIntLiteral(IntLiteralExpr* expr);
    llvm::Value* generateFloatLiteral(FloatLiteralExpr* expr);
    llvm::Value* generateBoolLiteral(BoolLiteralExpr* expr);
    llvm::Value* generateVariable(VariableExpr* expr);
    llvm::Value* generateBinaryExpr(BinaryExpr* expr);
    llvm::Value* generateUnaryExpr(UnaryExpr* expr);
    llvm::Value* generateCallExpr(CallExpr* expr);
    llvm::Value* generateArrayIndexExpr(ArrayIndexExpr* expr);
};

#endif
