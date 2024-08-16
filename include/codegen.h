#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>

class CodeGenerator {
public:
    CodeGenerator();
    void generate(ASTNode* root);

private:
    llvm::LLVMContext context;
    llvm::Module* module;
    llvm::IRBuilder<> builder;

    void generateNode(ASTNode* node);
};

#endif