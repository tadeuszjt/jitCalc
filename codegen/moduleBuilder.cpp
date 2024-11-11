#include "moduleBuilder.h"
#include <cmath>
#include <iterator>

#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/Verifier.h>

using namespace llvm;

ModuleBuilder::ModuleBuilder(LLVMContext &context, const std::string &name)
    : irBuilder(context)
    , irModule(std::make_unique<Module>(name, context))
    , curFn(nullptr)
{
    // Declare the printf function
    Function::Create(
            FunctionType::get(irBuilder.getInt32Ty(), {irBuilder.getPtrTy()}, true),
            Function::ExternalLinkage,
            "printf",
            irModule.get());
}

void ModuleBuilder::optimiseModule() {
    LoopAnalysisManager LAM;
    FunctionAnalysisManager FAM;
    CGSCCAnalysisManager CGAM;
    ModuleAnalysisManager MAM;

    PassBuilder PB;

    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(OptimizationLevel::O2);
    MPM.run(*irModule, MAM);
}

Value* ModuleBuilder::createCall(const std::string &name, const std::vector<Value*> &args) {
    return irBuilder.CreateCall(irModule->getFunction(name), args);
}

BasicBlock* ModuleBuilder::appendNewBlock(const std::string &suggestion) {
    assert(curFn != nullptr);
    return BasicBlock::Create(irBuilder.getContext(), suggestion.c_str(), curFn);
}

void ModuleBuilder::setCurrentBlock(BasicBlock *block) {
    assert(block != nullptr);
    irBuilder.SetInsertPoint(block);
}


llvm::BasicBlock* ModuleBuilder::getCurrentBlock() {
    llvm::BasicBlock *block = irBuilder.GetInsertBlock();
    assert(block != nullptr);
    return block;
}

llvm::BasicBlock* ModuleBuilder::getEntryBlock() {
    assert(curFn != nullptr);
    return &curFn->getEntryBlock();
}

Argument *ModuleBuilder::getCurrentFuncArg(size_t argIndex) {
    assert(curFn != nullptr);
    assert(argIndex < std::distance(curFn->args().begin(), curFn->args().end()));

    size_t index = 0;
    for (Argument &arg : curFn->args()) {
        if (index == argIndex) {
            return &arg;
        }
        index++;
    }
    assert(false);
}

void ModuleBuilder::createExtern(const std::string &name,
                                 const std::vector<Type*> &argTypes,
                                 Type *returnType) {
    FunctionType *fnType = FunctionType::get(returnType, argTypes, false);
    assert(fnType != nullptr);
    Function *fn = Function::Create(fnType, Function::ExternalLinkage, name, irModule.get());
    assert(fn != nullptr);
}

BasicBlock* ModuleBuilder::createFunction(const std::string &name, const std::vector<Type*> &argTypes, Type *returnType) {
    FunctionType *fnType = FunctionType::get(returnType, argTypes, false);
    assert(fnType != nullptr);
    Function *fn = Function::Create(fnType, Function::ExternalLinkage, name, irModule.get());
    assert(fn != nullptr);
    BasicBlock *block = BasicBlock::Create(irModule->getContext(), "EntryBlock", fn);
    assert(block != nullptr);
    return block;
}

void ModuleBuilder::setCurrentFunction(const std::string &name) {
    Function *fn = irModule->getFunction(name);
    assert(fn != nullptr);
    curFn = fn;
    irBuilder.SetInsertPoint(&fn->back());
}

void ModuleBuilder::printModule() {
    irModule->print(outs(), nullptr);
}

void ModuleBuilder::verifyModule() {
    bool failed = llvm::verifyModule(*irModule, &llvm::errs());
    assert(!failed);
}
