#include "codegen.h"
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>  // For target initialization
#include <optional>  // For std::optional

CodeGenerator::CodeGenerator() : builder(context) {
    // Initialize all targets
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    module = new llvm::Module("S-Lang Module", context);
}


void CodeGenerator::generate(ASTNode* root) {
    llvm::Value* result = generateNode(root);

    // Output the LLVM IR to a file
    std::error_code EC;
    llvm::raw_fd_ostream dest("output.ll", EC, llvm::sys::fs::OF_None);
    if (EC) {
        llvm::errs() << "Could not open file: " << EC.message();
        return;
    }

    module->print(dest, nullptr);
    dest.flush();

    // Generate object file
    llvm::legacy::PassManager passObj;
    auto TargetTriple = llvm::sys::getProcessTriple();
    module->setTargetTriple(TargetTriple);

    std::string Error;
    const llvm::Target* Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

    if (!Target) {
        llvm::errs() << Error;
        return;
    }

    auto CPU = "generic";
    auto Features = "";

    llvm::TargetOptions opt;
    auto TheTargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, std::nullopt);

    module->setDataLayout(TheTargetMachine->createDataLayout());

    std::error_code EC2;
    llvm::raw_fd_ostream destObj("output.o", EC2, llvm::sys::fs::OF_None);

    if (EC2) {
        llvm::errs() << "Could not open file: " << EC2.message();
        return;
    }

    llvm::CodeGenFileType FileType = static_cast<llvm::CodeGenFileType>(0);  // Assuming 0 corresponds to object file generation
    if (TheTargetMachine->addPassesToEmitFile(passObj, destObj, nullptr, FileType)) {
        llvm::errs() << "TheTargetMachine can't emit a file of this type";
        return;
    }

    passObj.run(*module);
    destObj.flush();

    llvm::outs() << "Generated object file: output.o\n";
}

llvm::Value* CodeGenerator::generateNode(ASTNode* node) {
    switch (node->type) {
        case ASTNodeType::Literal: {
            // Assuming the literal is an integer
            int value = std::stoi(node->value);
            return llvm::ConstantInt::get(context, llvm::APInt(32, value));
        }
        case ASTNodeType::BinaryOp: {
            llvm::Value* left = generateNode(node->children[0]);
            llvm::Value* right = generateNode(node->children[1]);

            if (node->value == "+") {
                return builder.CreateAdd(left, right, "addtmp");
            } else if (node->value == "-") {
                return builder.CreateSub(left, right, "subtmp");
            } else if (node->value == "*") {
                return builder.CreateMul(left, right, "multmp");
            } else if (node->value == "/") {
                return builder.CreateSDiv(left, right, "divtmp");
            }
            break;
        }
        case ASTNodeType::Variable: {
            llvm::Value* var = module->getGlobalVariable(node->value);
            if (var) {
                // Assuming the variable is of type int32
                return builder.CreateLoad(builder.getInt32Ty(), var, node->value.c_str());
            }
            break;
        }
        default:
            llvm::errs() << "Unknown AST node type\n";
            return nullptr;
    }
    return nullptr;
}
