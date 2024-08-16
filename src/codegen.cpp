#include "codegen.h"

CodeGenerator::CodeGenerator() : builder(context) {
    module = new llvm::Module("S-Lang Module", context);
}

void CodeGenerator::generate(ASTNode* root) {
    generateNode(root);
    module->print(llvm::errs(), nullptr);
}

void CodeGenerator::generateNode(ASTNode* node) {
    if (node->type == ASTNodeType::Literal) {
        llvm::Value* value = llvm::ConstantInt::get(context, llvm::APInt(32, std::stoi(node->value)));
        // More LLVM code generation...
    }
    // Handle other node types...
}