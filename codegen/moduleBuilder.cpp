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
    , funcDefCurrent(nullptr)
{
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


void ModuleBuilder::createGlobalDeclaration(const char* name, llvm::Type* type) {
    auto *globalVar = new GlobalVariable(
        *irModule,
        type,
        false,
        GlobalValue::ExternalLinkage,
        nullptr,
        name);
    assert(globalVar != nullptr);
}

llvm::GlobalVariable* ModuleBuilder::getGlobalVariable(const char* name) {
    return irModule->getGlobalVariable(name);
}

Value* ModuleBuilder::createCall(const char *name, const std::vector<Value*> &args) {
    return irBuilder.CreateCall(irModule->getFunction(name), args);
}

llvm::Value* ModuleBuilder::createInvoke(
    llvm::BasicBlock *normalDest,
    llvm::BasicBlock* unwindDest,
    const char *name,
    const std::vector<llvm::Value*> &args)
{
    return irBuilder.CreateInvoke(irModule->getFunction(name), normalDest, unwindDest, args);
}


void ModuleBuilder::createTrap() {
    Function *trap = Intrinsic::getOrInsertDeclaration(irModule.get(), Intrinsic::trap);
    irBuilder.CreateCall(trap);
}

BasicBlock* ModuleBuilder::appendNewBlock(const char*suggestion) {
    assert(nullptr != funcDefCurrent);
    return BasicBlock::Create(irBuilder.getContext(), suggestion, funcDefCurrent->fnPtr);
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

Argument *ModuleBuilder::getCurrentFuncArg(size_t argIndex) {
    assert(funcDefCurrent != nullptr);
    assert(argIndex < std::distance(funcDefCurrent->fnPtr->args().begin(), funcDefCurrent->fnPtr->args().end()));

    size_t index = 0;
    for (Argument &arg : funcDefCurrent->fnPtr->args()) {
        if (index == argIndex) {
            return &arg;
        }
        index++;
    }
    assert(false);
}

Function* ModuleBuilder::createFuncDeclaration(
    const std::string &name,
    Type* returnType,
    const std::vector<Type*> &argTypes,
    bool isVarg
) {
    assert(funcDefs.find(name) == funcDefs.end());
    Function *fn = Function::Create(
        FunctionType::get(returnType, argTypes, isVarg),
        Function::ExternalLinkage,
        name,
        irModule.get()
    );
    assert(nullptr != fn);
    funcDefs[name].fnPtr = fn;
    return fn;
}


Function* ModuleBuilder::createFunc(
    const std::string &name,
    const std::vector<Type*> &argTypes,
    Type *returnType
) {
    auto *fn = createFuncDeclaration(name, returnType, argTypes, false);
    BasicBlock *block = BasicBlock::Create(irModule->getContext(), "EntryBlock", fn);
    assert(block != nullptr);
    return fn;
}

void ModuleBuilder::setCurrentFunc(const std::string &name) {
    assert(funcDefs.find(name) != funcDefs.end());
    funcDefCurrent = &funcDefs[name];
    irBuilder.SetInsertPoint(&funcDefCurrent->fnPtr->back());
}

Function* ModuleBuilder::getFunc(const std::string &name) {
    if (funcDefs.find(name) == funcDefs.end()) {
        errs() << "function: " << name << " not created in module\n";
        assert(false);
    }

    return funcDefs[name].fnPtr;
}

void ModuleBuilder::printModule() {
    irModule->print(outs(), nullptr);
}

void ModuleBuilder::verifyModule() {
    bool failed = llvm::verifyModule(*irModule, &llvm::errs());
    assert(!failed);
}
