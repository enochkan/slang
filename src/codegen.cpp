#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

LLVMContext Context;
IRBuilder<> Builder(Context);
std::unique_ptr<Module> TheModule;

void initializeModuleAndPassManager() {
    TheModule = std::make_unique<Module>("my_slang_compiler", Context);
}

Function* createPrintFunction() {
    FunctionType* printType = FunctionType::get(Type::getVoidTy(Context), false);
    Function* function = Function::Create(printType, Function::ExternalLinkage, "print", TheModule.get());
    return function;
}

void generateIR(const ASTNode& tree) {
    initializeModuleAndPassManager();

    Function* printFunc = createPrintFunction();
    BasicBlock* BB = BasicBlock::Create(Context, "entry", printFunc);
    Builder.SetInsertPoint(BB);

    if (tree.type == "PRINT") {
        // Call the "print" function
        Builder.CreateCall(printFunc);
        Builder.CreateRetVoid();
    }

    verifyFunction(*printFunc);
    TheModule->print(outs(), nullptr);
}

void compileAndGenerateObjectFile() {
    std::string TargetTriple = sys::getDefaultTargetTriple();
    TheModule->setTargetTriple(TargetTriple);

    std::string Error;
    const Target* Target = TargetRegistry::lookupTarget(TargetTriple, Error);

    if (!Target) {
        errs() << "Error: " << Error;
        return;
    }

    auto CPU = "generic";
    auto Features = "";

    TargetOptions opt;
    auto RM = Optional<Reloc::Model>();
    auto TheTargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

    TheModule->setDataLayout(TheTargetMachine->createDataLayout());

    std::string Filename = "output.o";
    std::error_code EC;
    raw_fd_ostream dest(Filename, EC, sys::fs::OF_None);

    if (EC) {
        errs() << "Could not open file: " << EC.message();
        return;
    }

    legacy::PassManager pass;
    auto FileType = CGFT_ObjectFile;

    if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
        errs() << "TheTargetMachine can't emit a file of this type";
        return;
    }

    pass.run(*TheModule);
    dest.flush();

    outs() << "Wrote " << Filename << "\n";
}