#include "codegen.h"
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/MC/TargetRegistry.h>
#include <optional>  // For std::optional

CodeGenerator::CodeGenerator() : builder(context) {
    module = new llvm::Module("S-Lang Module", context);
}

void CodeGenerator::generate(ASTNode* root) {
    generateNode(root);

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

    // Updated code to use the enum value directly or based on fallback approach
    llvm::CodeGenFileType FileType = static_cast<llvm::CodeGenFileType>(0);  // Assuming 0 corresponds to object file generation
    if (TheTargetMachine->addPassesToEmitFile(passObj, destObj, nullptr, FileType)) {
        llvm::errs() << "TheTargetMachine can't emit a file of this type";
        return;
    }

    passObj.run(*module);
    destObj.flush();

    llvm::outs() << "Generated object file: output.o\n";
}
