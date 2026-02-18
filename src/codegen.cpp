#include "codegen.h"
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/TargetParser/SubtargetFeature.h>
#include <optional>

CodeGenerator::CodeGenerator() : builder(context) {
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    module = std::make_unique<llvm::Module>("S-Lang Module", context);
}

void CodeGenerator::declarePrintf() {
    auto printfType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context),
        {llvm::PointerType::getUnqual(context)},
        true // variadic
    );
    printfFunc = module->getOrInsertFunction("printf", printfType);
}

llvm::Type* CodeGenerator::getLLVMType(SlangType type) {
    switch (type) {
        case SlangType::I32:  return llvm::Type::getInt32Ty(context);
        case SlangType::F64:  return llvm::Type::getDoubleTy(context);
        case SlangType::Bool: return llvm::Type::getInt1Ty(context);
        case SlangType::Void: return llvm::Type::getVoidTy(context);
    }
    return llvm::Type::getVoidTy(context);
}

llvm::AllocaInst* CodeGenerator::createEntryBlockAlloca(llvm::Function* fn, const std::string& name, llvm::Type* type) {
    llvm::IRBuilder<> tmpBuilder(&fn->getEntryBlock(), fn->getEntryBlock().begin());
    return tmpBuilder.CreateAlloca(type, nullptr, name);
}

SlangType CodeGenerator::inferExprType(ExprNode* expr) {
    if (dynamic_cast<IntLiteralExpr*>(expr)) return SlangType::I32;
    if (dynamic_cast<FloatLiteralExpr*>(expr)) return SlangType::F64;
    if (dynamic_cast<BoolLiteralExpr*>(expr)) return SlangType::Bool;
    if (auto* var = dynamic_cast<VariableExpr*>(expr)) {
        auto it = namedValues.find(var->name);
        if (it != namedValues.end()) return it->second.type;
        return SlangType::I32;
    }
    if (auto* bin = dynamic_cast<BinaryExpr*>(expr)) {
        if (bin->op == "==" || bin->op == "!=" || bin->op == "<" || bin->op == ">" ||
            bin->op == "<=" || bin->op == ">=" || bin->op == "&&" || bin->op == "||") {
            return SlangType::Bool;
        }
        return inferExprType(bin->left.get());
    }
    if (auto* unary = dynamic_cast<UnaryExpr*>(expr)) {
        if (unary->op == "!") return SlangType::Bool;
        return inferExprType(unary->operand.get());
    }
    if (auto* idx = dynamic_cast<ArrayIndexExpr*>(expr)) {
        auto it = namedValues.find(idx->name);
        if (it != namedValues.end()) return it->second.elemType;
        return SlangType::I32;
    }
    if (auto* fa = dynamic_cast<FieldAccessExpr*>(expr)) {
        auto it = namedValues.find(fa->varName);
        if (it != namedValues.end()) {
            int fidx = getFieldIndex(it->second.structName, fa->field);
            if (fidx >= 0) return structDefs[it->second.structName][fidx].type;
        }
        return SlangType::I32;
    }
    if (auto* call = dynamic_cast<CallExpr*>(expr)) {
        auto* fn = module->getFunction(call->callee);
        if (fn) {
            if (fn->getReturnType()->isIntegerTy(32)) return SlangType::I32;
            if (fn->getReturnType()->isDoubleTy()) return SlangType::F64;
            if (fn->getReturnType()->isIntegerTy(1)) return SlangType::Bool;
        }
        return SlangType::I32;
    }
    return SlangType::I32;
}

// --- Main entry ---

void CodeGenerator::generateStructDecl(StructDecl& s) {
    structDefs[s.name] = s.fields;
    std::vector<llvm::Type*> fieldTypes;
    for (auto& f : s.fields) fieldTypes.push_back(getLLVMType(f.type));
    llvmStructTypes[s.name] = llvm::StructType::create(context, fieldTypes, s.name);
}

int CodeGenerator::getFieldIndex(const std::string& structName, const std::string& fieldName) {
    auto& fields = structDefs[structName];
    for (int i = 0; i < (int)fields.size(); i++) {
        if (fields[i].name == fieldName) return i;
    }
    return -1;
}

// SoA LLVM type: { [N x f0], [N x f1], ... } — one sub-array per struct field.
// This is the key layout that enables auto-vectorisation on field-parallel loops.
llvm::StructType* CodeGenerator::getSoAType(const std::string& structName, int size) {
    std::vector<llvm::Type*> soaFields;
    for (auto& f : structDefs[structName])
        soaFields.push_back(llvm::ArrayType::get(getLLVMType(f.type), size));
    return llvm::StructType::get(context, soaFields);
}

void CodeGenerator::generate(Program& program, const std::string& outputBaseName) {
    // Register struct types before everything else
    for (auto& s : program.structs) generateStructDecl(*s);

    declarePrintf();

    for (auto& fn : program.functions) {
        generateFnDecl(*fn);
    }

    // Set up the native target machine
    llvm::Triple targetTriple(llvm::sys::getProcessTriple());
    module->setTargetTriple(targetTriple);

    std::string error;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
    if (!target) {
        llvm::errs() << "Target lookup failed: " << error << "\n";
        return;
    }

    // Use the host CPU name and all its features (AVX2, AVX512, NEON, etc.)
    // rather than "generic" - this enables vectorization on the actual hardware
    std::string cpuName = std::string(llvm::sys::getHostCPUName());
    // LLVM 21+: getHostCPUFeatures() returns a value instead of taking a reference
    auto hostFeatures = llvm::sys::getHostCPUFeatures();
    llvm::SubtargetFeatures subtargetFeatures;
    for (auto& kv : hostFeatures) {
        subtargetFeatures.AddFeature(kv.first(), kv.second);
    }

    llvm::TargetOptions targetOpts;
    // LLVM 21+: createTargetMachine returns a raw pointer
    std::unique_ptr<llvm::TargetMachine> TM(target->createTargetMachine(
        targetTriple,
        cpuName,
        subtargetFeatures.getString(),
        targetOpts,
        std::nullopt,                       // Reloc model (default)
        std::nullopt,                       // Code model (default)
        llvm::CodeGenOptLevel::Aggressive   // -O3 equivalent
    ));
    module->setDataLayout(TM->createDataLayout());

    // Run O3 + loop vectorization + SLP vectorization passes
    runOptimizationPasses(TM.get());

    emitIR(outputBaseName + ".ll");
    emitObjectFile(outputBaseName + ".o", TM.get());
}

void CodeGenerator::runOptimizationPasses(llvm::TargetMachine* TM) {
    llvm::PassBuilder PB(TM);

    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;

    // Register all standard analyses with their managers
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    // O3 pipeline: includes inlining, loop vectorization, SLP vectorization,
    // dead code elimination, and all standard scalar optimizations
    llvm::ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);
    MPM.run(*module, MAM);
}

void CodeGenerator::emitIR(const std::string& filename) {
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
    if (EC) {
        llvm::errs() << "Could not open file: " << EC.message() << "\n";
        return;
    }
    module->print(dest, nullptr);
    dest.flush();
}

void CodeGenerator::emitObjectFile(const std::string& filename, llvm::TargetMachine* TM) {
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
    if (EC) {
        llvm::errs() << "Could not open file: " << EC.message() << "\n";
        return;
    }

    // Legacy pass manager is still required for addPassesToEmitFile
    llvm::legacy::PassManager pass;
    auto fileType = llvm::CodeGenFileType::ObjectFile;
    if (TM->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
        llvm::errs() << "Target machine can't emit object file\n";
        return;
    }

    pass.run(*module);
    dest.flush();

    llvm::outs() << "Generated: " << filename << "\n";
}

// --- Function Declaration ---

void CodeGenerator::generateFnDecl(FnDecl& fn) {
    // Build parameter types
    std::vector<llvm::Type*> paramTypes;
    for (auto& param : fn.params) {
        paramTypes.push_back(getLLVMType(param.type));
    }

    auto* funcType = llvm::FunctionType::get(getLLVMType(fn.returnType), paramTypes, false);
    auto* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, fn.name, module.get());

    // Name the parameters
    unsigned idx = 0;
    for (auto& arg : func->args()) {
        arg.setName(fn.params[idx].name);
        idx++;
    }

    // Create entry block
    auto* entryBlock = llvm::BasicBlock::Create(context, "entry", func);
    builder.SetInsertPoint(entryBlock);

    // Clear symbol table for this function
    namedValues.clear();

    // Create allocas for parameters
    idx = 0;
    for (auto& arg : func->args()) {
        auto* alloca = createEntryBlockAlloca(func, fn.params[idx].name, getLLVMType(fn.params[idx].type));
        builder.CreateStore(&arg, alloca);
        namedValues[fn.params[idx].name] = {alloca, fn.params[idx].type, false};
        idx++;
    }

    // Generate body
    for (auto& stmt : fn.body->statements) {
        generateStmt(stmt.get());
    }

    // If the function returns void and there's no terminator, add ret void
    if (fn.returnType == SlangType::Void) {
        auto* currentBlock = builder.GetInsertBlock();
        if (!currentBlock->getTerminator()) {
            builder.CreateRetVoid();
        }
    }

    llvm::verifyFunction(*func);
}

// --- Statements ---

void CodeGenerator::generateStmt(StmtNode* stmt) {
    // Skip if current block already has a terminator
    if (builder.GetInsertBlock()->getTerminator()) return;

    if (auto* s = dynamic_cast<LetStmt*>(stmt))    return generateLetStmt(s);
    if (auto* s = dynamic_cast<AssignStmt*>(stmt))  return generateAssignStmt(s);
    if (auto* s = dynamic_cast<ReturnStmt*>(stmt))  return generateReturnStmt(s);
    if (auto* s = dynamic_cast<ExprStmt*>(stmt))    return generateExprStmt(s);
    if (auto* s = dynamic_cast<PrintStmt*>(stmt))   return generatePrintStmt(s);
    if (auto* s = dynamic_cast<BlockStmt*>(stmt))   return generateBlockStmt(s);
    if (auto* s = dynamic_cast<IfStmt*>(stmt))         return generateIfStmt(s);
    if (auto* s = dynamic_cast<WhileStmt*>(stmt))      return generateWhileStmt(s);
    if (auto* s = dynamic_cast<ForStmt*>(stmt))          return generateForStmt(s);
    if (auto* s = dynamic_cast<ArrayAssignStmt*>(stmt))  return generateArrayAssignStmt(s);
    if (auto* s = dynamic_cast<FieldAssignStmt*>(stmt))  return generateFieldAssignStmt(s);
}

void CodeGenerator::generateLetStmt(LetStmt* stmt) {
    auto* func = builder.GetInsertBlock()->getParent();

    // Scalar struct: let p: Point = Point { x: 1.0, y: 2.0 }
    if (stmt->type == SlangType::Struct) {
        auto* sType = llvmStructTypes[stmt->structName];
        auto* alloca = createEntryBlockAlloca(func, stmt->name, sType);
        if (auto* init = dynamic_cast<StructInitExpr*>(stmt->initializer.get())) {
            for (auto& [fieldName, fieldExpr] : init->fields) {
                int idx = getFieldIndex(stmt->structName, fieldName);
                if (idx < 0) continue;
                llvm::Value* val = generateExpr(fieldExpr.get());
                auto* gep = builder.CreateGEP(sType, alloca,
                    {builder.getInt32(0), builder.getInt32(idx)}, "field.init");
                builder.CreateStore(val, gep);
            }
        }
        namedValues[stmt->name] = {alloca, SlangType::Struct, stmt->isMutable,
                                   SlangType::I32, -1, stmt->structName};
        return;
    }

    // SoA struct-array: let [mut] ps: [Point; N]  — zero-initialised by default
    if (stmt->type == SlangType::Array && stmt->elemType == SlangType::Struct) {
        auto* soaType = getSoAType(stmt->structName, stmt->arraySize);
        auto* alloca = createEntryBlockAlloca(func, stmt->name, soaType);
        builder.CreateStore(llvm::Constant::getNullValue(soaType), alloca);
        namedValues[stmt->name] = {alloca, SlangType::Array, stmt->isMutable,
                                   SlangType::Struct, stmt->arraySize, stmt->structName};
        return;
    }

    // Scalar-array declaration: let [mut] arr: [elemType; N] = [v0, v1, ...]
    if (stmt->type == SlangType::Array) {
        auto* elemLLVMType = getLLVMType(stmt->elemType);
        auto* arrayType = llvm::ArrayType::get(elemLLVMType, stmt->arraySize);
        auto* alloca = createEntryBlockAlloca(func, stmt->name, arrayType);

        if (auto* arrLit = dynamic_cast<ArrayLiteralExpr*>(stmt->initializer.get())) {
            for (int i = 0; i < (int)arrLit->elements.size(); i++) {
                llvm::Value* elemVal = generateExpr(arrLit->elements[i].get());
                auto* gep = builder.CreateGEP(arrayType, alloca,
                    {builder.getInt32(0), builder.getInt32(i)}, "arr.init");
                builder.CreateStore(elemVal, gep);
            }
        }

        namedValues[stmt->name] = {alloca, SlangType::Array, stmt->isMutable,
                                   stmt->elemType, stmt->arraySize};
        return;
    }

    // Scalar declaration
    auto* type = getLLVMType(stmt->type);
    auto* alloca = createEntryBlockAlloca(func, stmt->name, type);
    llvm::Value* initVal = generateExpr(stmt->initializer.get());

    // Type conversion if needed
    if (stmt->type == SlangType::F64 && initVal->getType()->isIntegerTy(32)) {
        initVal = builder.CreateSIToFP(initVal, llvm::Type::getDoubleTy(context), "conv");
    } else if (stmt->type == SlangType::I32 && initVal->getType()->isDoubleTy()) {
        initVal = builder.CreateFPToSI(initVal, llvm::Type::getInt32Ty(context), "conv");
    }

    builder.CreateStore(initVal, alloca);
    namedValues[stmt->name] = {alloca, stmt->type, stmt->isMutable};
}

void CodeGenerator::generateAssignStmt(AssignStmt* stmt) {
    auto it = namedValues.find(stmt->name);
    if (it == namedValues.end()) {
        llvm::errs() << "Unknown variable: " << stmt->name << "\n";
        return;
    }
    if (!it->second.isMutable) {
        llvm::errs() << "Cannot assign to immutable variable: " << stmt->name << "\n";
        return;
    }

    llvm::Value* val = generateExpr(stmt->value.get());
    builder.CreateStore(val, it->second.alloca);
}

void CodeGenerator::generateReturnStmt(ReturnStmt* stmt) {
    llvm::Value* val = generateExpr(stmt->value.get());
    builder.CreateRet(val);
}

void CodeGenerator::generateExprStmt(ExprStmt* stmt) {
    generateExpr(stmt->expr.get());
}

void CodeGenerator::generatePrintStmt(PrintStmt* stmt) {
    llvm::Value* val = generateExpr(stmt->expr.get());
    SlangType exprType = inferExprType(stmt->expr.get());

    llvm::Value* formatStr;
    std::vector<llvm::Value*> args;

    if (exprType == SlangType::F64 || val->getType()->isDoubleTy()) {
        formatStr = builder.CreateGlobalString("%f\n", "fmt_f64");
        args = {formatStr, val};
    } else if (exprType == SlangType::Bool && val->getType()->isIntegerTy(1)) {
        // Extend bool to i32 for printf
        val = builder.CreateZExt(val, llvm::Type::getInt32Ty(context), "boolext");
        formatStr = builder.CreateGlobalString("%d\n", "fmt_bool");
        args = {formatStr, val};
    } else {
        formatStr = builder.CreateGlobalString("%d\n", "fmt_i32");
        args = {formatStr, val};
    }

    builder.CreateCall(printfFunc, args);
}

void CodeGenerator::generateBlockStmt(BlockStmt* stmt) {
    for (auto& s : stmt->statements) {
        generateStmt(s.get());
    }
}

void CodeGenerator::generateIfStmt(IfStmt* stmt) {
    llvm::Value* condVal = generateExpr(stmt->condition.get());

    // Convert to i1 if needed
    if (condVal->getType()->isIntegerTy(32)) {
        condVal = builder.CreateICmpNE(condVal, llvm::ConstantInt::get(context, llvm::APInt(32, 0)), "ifcond");
    }

    auto* func = builder.GetInsertBlock()->getParent();
    auto* thenBB = llvm::BasicBlock::Create(context, "then", func);
    auto* mergeBB = llvm::BasicBlock::Create(context, "ifcont");
    llvm::BasicBlock* elseBB = nullptr;

    if (stmt->elseBlock) {
        elseBB = llvm::BasicBlock::Create(context, "else");
        builder.CreateCondBr(condVal, thenBB, elseBB);
    } else {
        builder.CreateCondBr(condVal, thenBB, mergeBB);
    }

    // Then block
    builder.SetInsertPoint(thenBB);
    generateBlockStmt(stmt->thenBlock.get());
    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(mergeBB);
    }

    // Else block
    if (elseBB) {
        func->insert(func->end(), elseBB);
        builder.SetInsertPoint(elseBB);
        generateBlockStmt(stmt->elseBlock.get());
        if (!builder.GetInsertBlock()->getTerminator()) {
            builder.CreateBr(mergeBB);
        }
    }

    // Merge block
    func->insert(func->end(), mergeBB);
    builder.SetInsertPoint(mergeBB);
}

void CodeGenerator::generateWhileStmt(WhileStmt* stmt) {
    auto* func = builder.GetInsertBlock()->getParent();
    auto* condBB = llvm::BasicBlock::Create(context, "whilecond", func);
    auto* bodyBB = llvm::BasicBlock::Create(context, "whilebody");
    auto* afterBB = llvm::BasicBlock::Create(context, "whileafter");

    builder.CreateBr(condBB);

    // Condition block
    builder.SetInsertPoint(condBB);
    llvm::Value* condVal = generateExpr(stmt->condition.get());
    if (condVal->getType()->isIntegerTy(32)) {
        condVal = builder.CreateICmpNE(condVal, llvm::ConstantInt::get(context, llvm::APInt(32, 0)), "whilecond");
    }
    builder.CreateCondBr(condVal, bodyBB, afterBB);

    // Body block
    func->insert(func->end(), bodyBB);
    builder.SetInsertPoint(bodyBB);
    generateBlockStmt(stmt->body.get());
    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(condBB);
    }

    // After block
    func->insert(func->end(), afterBB);
    builder.SetInsertPoint(afterBB);
}

void CodeGenerator::generateArrayAssignStmt(ArrayAssignStmt* stmt) {
    auto it = namedValues.find(stmt->name);
    if (it == namedValues.end()) {
        llvm::errs() << "Unknown array variable: " << stmt->name << "\n";
        return;
    }
    if (!it->second.isMutable) {
        llvm::errs() << "Cannot assign to immutable array: " << stmt->name << "\n";
        return;
    }

    auto& info = it->second;
    auto* arrayType = llvm::ArrayType::get(getLLVMType(info.elemType), info.arraySize);
    llvm::Value* idx = generateExpr(stmt->index.get());
    llvm::Value* val = generateExpr(stmt->value.get());

    auto* gep = builder.CreateGEP(arrayType, info.alloca,
        {builder.getInt32(0), idx}, "arr.idx");
    builder.CreateStore(val, gep);
}

void CodeGenerator::generateFieldAssignStmt(FieldAssignStmt* stmt) {
    auto it = namedValues.find(stmt->varName);
    if (it == namedValues.end()) {
        llvm::errs() << "Unknown variable: " << stmt->varName << "\n"; return;
    }
    auto& info = it->second;
    if (!info.isMutable) {
        llvm::errs() << "Cannot assign to immutable variable: " << stmt->varName << "\n"; return;
    }

    int fieldIdx = getFieldIndex(info.structName, stmt->field);
    if (fieldIdx < 0) {
        llvm::errs() << "Unknown field: " << stmt->field << "\n"; return;
    }
    SlangType fieldType = structDefs[info.structName][fieldIdx].type;
    llvm::Value* val = generateExpr(stmt->value.get());

    if (stmt->arrayIndex == nullptr) {
        // Scalar struct: p.field = val
        auto* gep = builder.CreateGEP(llvmStructTypes[info.structName], info.alloca,
            {builder.getInt32(0), builder.getInt32(fieldIdx)}, "field.ptr");
        builder.CreateStore(val, gep);
    } else {
        // SoA: points[i].field = val
        // Layout: { [N x f0type], [N x f1type], ... }
        // GEP: outer struct field fieldIdx, then element index i
        (void)fieldType;
        llvm::Value* idx = generateExpr(stmt->arrayIndex.get());
        auto* soaType = getSoAType(info.structName, info.arraySize);
        auto* gep = builder.CreateGEP(soaType, info.alloca,
            {builder.getInt32(0), builder.getInt32(fieldIdx), idx}, "soa.field.ptr");
        builder.CreateStore(val, gep);
    }
}

void CodeGenerator::generateForStmt(ForStmt* stmt) {
    auto* func = builder.GetInsertBlock()->getParent();
    auto* i32Ty = llvm::Type::getInt32Ty(context);

    // Allocate and initialise the loop variable
    auto* iAlloca = createEntryBlockAlloca(func, stmt->varName, i32Ty);
    llvm::Value* startVal = generateExpr(stmt->start.get());
    builder.CreateStore(startVal, iAlloca);

    // Register as immutable (loop variable can't be reassigned by the user)
    namedValues[stmt->varName] = {iAlloca, SlangType::I32, false};

    auto* condBB  = llvm::BasicBlock::Create(context, "forcond", func);
    auto* bodyBB  = llvm::BasicBlock::Create(context, "forbody");
    auto* afterBB = llvm::BasicBlock::Create(context, "forafter");

    builder.CreateBr(condBB);

    // Condition: i < end (end is re-evaluated each iteration for dynamic ranges)
    builder.SetInsertPoint(condBB);
    llvm::Value* endVal = generateExpr(stmt->end.get());
    llvm::Value* iVal   = builder.CreateLoad(i32Ty, iAlloca, stmt->varName);
    llvm::Value* cond   = builder.CreateICmpSLT(iVal, endVal, "forcond");
    builder.CreateCondBr(cond, bodyBB, afterBB);

    // Body
    func->insert(func->end(), bodyBB);
    builder.SetInsertPoint(bodyBB);
    generateBlockStmt(stmt->body.get());

    // Increment i = i + 1 (unless body already has a terminator)
    if (!builder.GetInsertBlock()->getTerminator()) {
        llvm::Value* iCurr = builder.CreateLoad(i32Ty, iAlloca, "i.curr");
        llvm::Value* iNext = builder.CreateAdd(iCurr, builder.getInt32(1), "i.next");
        builder.CreateStore(iNext, iAlloca);
        builder.CreateBr(condBB);
    }

    // After loop
    func->insert(func->end(), afterBB);
    builder.SetInsertPoint(afterBB);

    namedValues.erase(stmt->varName);
}

// --- Expressions ---

llvm::Value* CodeGenerator::generateExpr(ExprNode* expr) {
    if (auto* e = dynamic_cast<IntLiteralExpr*>(expr))   return generateIntLiteral(e);
    if (auto* e = dynamic_cast<FloatLiteralExpr*>(expr)) return generateFloatLiteral(e);
    if (auto* e = dynamic_cast<BoolLiteralExpr*>(expr))  return generateBoolLiteral(e);
    if (auto* e = dynamic_cast<VariableExpr*>(expr))     return generateVariable(e);
    if (auto* e = dynamic_cast<BinaryExpr*>(expr))       return generateBinaryExpr(e);
    if (auto* e = dynamic_cast<UnaryExpr*>(expr))        return generateUnaryExpr(e);
    if (auto* e = dynamic_cast<CallExpr*>(expr))           return generateCallExpr(e);
    if (auto* e = dynamic_cast<ArrayIndexExpr*>(expr))     return generateArrayIndexExpr(e);
    if (auto* e = dynamic_cast<FieldAccessExpr*>(expr))    return generateFieldAccessExpr(e);

    llvm::errs() << "Unknown expression type\n";
    return nullptr;
}

llvm::Value* CodeGenerator::generateIntLiteral(IntLiteralExpr* expr) {
    return llvm::ConstantInt::get(context, llvm::APInt(32, expr->value, true));
}

llvm::Value* CodeGenerator::generateFloatLiteral(FloatLiteralExpr* expr) {
    return llvm::ConstantFP::get(context, llvm::APFloat(expr->value));
}

llvm::Value* CodeGenerator::generateBoolLiteral(BoolLiteralExpr* expr) {
    return llvm::ConstantInt::get(context, llvm::APInt(1, expr->value ? 1 : 0));
}

llvm::Value* CodeGenerator::generateVariable(VariableExpr* expr) {
    auto it = namedValues.find(expr->name);
    if (it == namedValues.end()) {
        llvm::errs() << "Unknown variable: " << expr->name << "\n";
        return nullptr;
    }
    return builder.CreateLoad(getLLVMType(it->second.type), it->second.alloca, expr->name);
}

llvm::Value* CodeGenerator::generateBinaryExpr(BinaryExpr* expr) {
    llvm::Value* left = generateExpr(expr->left.get());
    llvm::Value* right = generateExpr(expr->right.get());

    if (!left || !right) return nullptr;

    // Logical operators (short-circuit not implemented for simplicity)
    if (expr->op == "&&") {
        // Both operands to i1
        if (left->getType()->isIntegerTy(32))
            left = builder.CreateICmpNE(left, llvm::ConstantInt::get(context, llvm::APInt(32, 0)), "tobool");
        if (right->getType()->isIntegerTy(32))
            right = builder.CreateICmpNE(right, llvm::ConstantInt::get(context, llvm::APInt(32, 0)), "tobool");
        return builder.CreateAnd(left, right, "andtmp");
    }
    if (expr->op == "||") {
        if (left->getType()->isIntegerTy(32))
            left = builder.CreateICmpNE(left, llvm::ConstantInt::get(context, llvm::APInt(32, 0)), "tobool");
        if (right->getType()->isIntegerTy(32))
            right = builder.CreateICmpNE(right, llvm::ConstantInt::get(context, llvm::APInt(32, 0)), "tobool");
        return builder.CreateOr(left, right, "ortmp");
    }

    bool isFloat = left->getType()->isDoubleTy() || right->getType()->isDoubleTy();

    // Promote to float if mixed
    if (isFloat) {
        if (left->getType()->isIntegerTy(32))
            left = builder.CreateSIToFP(left, llvm::Type::getDoubleTy(context), "conv");
        if (right->getType()->isIntegerTy(32))
            right = builder.CreateSIToFP(right, llvm::Type::getDoubleTy(context), "conv");
    }

    // Arithmetic operators
    if (expr->op == "+") {
        return isFloat ? builder.CreateFAdd(left, right, "addtmp")
                       : builder.CreateAdd(left, right, "addtmp");
    }
    if (expr->op == "-") {
        return isFloat ? builder.CreateFSub(left, right, "subtmp")
                       : builder.CreateSub(left, right, "subtmp");
    }
    if (expr->op == "*") {
        return isFloat ? builder.CreateFMul(left, right, "multmp")
                       : builder.CreateMul(left, right, "multmp");
    }
    if (expr->op == "/") {
        return isFloat ? builder.CreateFDiv(left, right, "divtmp")
                       : builder.CreateSDiv(left, right, "divtmp");
    }

    // Comparison operators
    if (expr->op == "==") {
        return isFloat ? builder.CreateFCmpOEQ(left, right, "eqtmp")
                       : builder.CreateICmpEQ(left, right, "eqtmp");
    }
    if (expr->op == "!=") {
        return isFloat ? builder.CreateFCmpONE(left, right, "netmp")
                       : builder.CreateICmpNE(left, right, "netmp");
    }
    if (expr->op == "<") {
        return isFloat ? builder.CreateFCmpOLT(left, right, "lttmp")
                       : builder.CreateICmpSLT(left, right, "lttmp");
    }
    if (expr->op == ">") {
        return isFloat ? builder.CreateFCmpOGT(left, right, "gttmp")
                       : builder.CreateICmpSGT(left, right, "gttmp");
    }
    if (expr->op == "<=") {
        return isFloat ? builder.CreateFCmpOLE(left, right, "letmp")
                       : builder.CreateICmpSLE(left, right, "letmp");
    }
    if (expr->op == ">=") {
        return isFloat ? builder.CreateFCmpOGE(left, right, "getmp")
                       : builder.CreateICmpSGE(left, right, "getmp");
    }

    llvm::errs() << "Unknown binary operator: " << expr->op << "\n";
    return nullptr;
}

llvm::Value* CodeGenerator::generateUnaryExpr(UnaryExpr* expr) {
    llvm::Value* operand = generateExpr(expr->operand.get());
    if (!operand) return nullptr;

    if (expr->op == "-") {
        if (operand->getType()->isDoubleTy()) {
            return builder.CreateFNeg(operand, "negtmp");
        }
        return builder.CreateNeg(operand, "negtmp");
    }
    if (expr->op == "!") {
        if (operand->getType()->isIntegerTy(32)) {
            operand = builder.CreateICmpNE(operand, llvm::ConstantInt::get(context, llvm::APInt(32, 0)), "tobool");
        }
        return builder.CreateNot(operand, "nottmp");
    }

    llvm::errs() << "Unknown unary operator: " << expr->op << "\n";
    return nullptr;
}

llvm::Value* CodeGenerator::generateArrayIndexExpr(ArrayIndexExpr* expr) {
    auto it = namedValues.find(expr->name);
    if (it == namedValues.end()) {
        llvm::errs() << "Unknown array variable: " << expr->name << "\n";
        return nullptr;
    }

    auto& info = it->second;
    auto* elemLLVMType = getLLVMType(info.elemType);
    auto* arrayType = llvm::ArrayType::get(elemLLVMType, info.arraySize);
    llvm::Value* idx = generateExpr(expr->index.get());

    auto* gep = builder.CreateGEP(arrayType, info.alloca,
        {builder.getInt32(0), idx}, "arr.idx");
    return builder.CreateLoad(elemLLVMType, gep, "arr.load");
}

llvm::Value* CodeGenerator::generateFieldAccessExpr(FieldAccessExpr* expr) {
    auto it = namedValues.find(expr->varName);
    if (it == namedValues.end()) {
        llvm::errs() << "Unknown variable: " << expr->varName << "\n"; return nullptr;
    }
    auto& info = it->second;

    int fieldIdx = getFieldIndex(info.structName, expr->field);
    if (fieldIdx < 0) {
        llvm::errs() << "Unknown field: " << expr->field << "\n"; return nullptr;
    }
    SlangType fieldType = structDefs[info.structName][fieldIdx].type;
    auto* elemLLVMType = getLLVMType(fieldType);

    if (expr->arrayIndex == nullptr) {
        // Scalar struct read: p.field
        auto* gep = builder.CreateGEP(llvmStructTypes[info.structName], info.alloca,
            {builder.getInt32(0), builder.getInt32(fieldIdx)}, "field.ptr");
        return builder.CreateLoad(elemLLVMType, gep, "field.load");
    } else {
        // SoA read: points[i].field
        llvm::Value* idx = generateExpr(expr->arrayIndex.get());
        auto* soaType = getSoAType(info.structName, info.arraySize);
        auto* gep = builder.CreateGEP(soaType, info.alloca,
            {builder.getInt32(0), builder.getInt32(fieldIdx), idx}, "soa.field.ptr");
        return builder.CreateLoad(elemLLVMType, gep, "soa.field.load");
    }
}

llvm::Value* CodeGenerator::generateCallExpr(CallExpr* expr) {
    auto* callee = module->getFunction(expr->callee);
    if (!callee) {
        llvm::errs() << "Unknown function: " << expr->callee << "\n";
        return nullptr;
    }

    std::vector<llvm::Value*> args;
    for (auto& arg : expr->args) {
        llvm::Value* val = generateExpr(arg.get());
        if (!val) return nullptr;
        args.push_back(val);
    }

    return builder.CreateCall(callee, args, "calltmp");
}
