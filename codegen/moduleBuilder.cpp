#include "moduleBuilder.h"
#include <cmath>
#include <iterator>

#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/Verifier.h>

using namespace llvm;

ModuleBuilder::ModuleBuilder(
    LLVMContext &context,
    const std::string &name,
    const std::filesystem::path &debugFilePath
)
    : irBuilder(context)
    , irModule(std::make_unique<Module>(name, context))
    , funcDefCurrent("")
    , diBuilder(*irModule)
{
    // create debug structures
    diFile = diBuilder.createFile(
        debugFilePath.filename().c_str(),
        debugFilePath.parent_path().c_str()
    );
    assert(nullptr != diFile);

    diCompileUnit = diBuilder.createCompileUnit(
        llvm::dwarf::DW_LANG_Modula2,
        diFile,
        "jitCalc",
        false,
        "",
        0,
        "",
        llvm::DICompileUnit::DebugEmissionKind::FullDebug
    );
    assert(nullptr != diCompileUnit);

    //diInt32Ty = diBuilder.createBasicType("INTEGER", 32, llvm::dwarf::DW_ATE_signed);
    //assert(nullptr != diInt32Ty);
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
    assert(funcDefs.find(name) != funcDefs.end());
    auto *call = irBuilder.CreateCall(irModule->getFunction(name), args);

    std::string curFn = funcDefCurrent;

    if (nullptr != funcDefs[curFn].diFunc) {
        auto *diLoc = llvm::DILocation::get(irModule->getContext(), 0, 0, funcDefs[curFn].diFunc);
        call->setDebugLoc(diLoc);
    }

    return call;
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
    assert(funcDefs.find(funcDefCurrent) != funcDefs.end());
    return BasicBlock::Create(irBuilder.getContext(), suggestion, funcDefs[funcDefCurrent].fnPtr);
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
    assert(funcDefs.find(funcDefCurrent) != funcDefs.end());
    auto *fnPtr = funcDefs[funcDefCurrent].fnPtr;

    assert(argIndex < std::distance(fnPtr->args().begin(), fnPtr->args().end()));

    size_t index = 0;
    for (Argument &arg : fnPtr->args()) {
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
    funcDefs[name].diFunc = nullptr;

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

    auto *diInt32 = diBuilder.createBasicType("INTEGER", 32, llvm::dwarf::DW_ATE_signed);

    std::vector<llvm::Metadata*> diArgTypes;
    diArgTypes.push_back(diInt32); // return
    for (llvm::Type *argType : argTypes) {
        diArgTypes.push_back(diInt32);
    }

    auto *diFuncType = diBuilder.createSubroutineType(
        diBuilder.getOrCreateTypeArray(diArgTypes)
    );

    assert(nullptr != diCompileUnit);
    funcDefs[name].diFunc = diBuilder.createFunction(
        diCompileUnit,
        name,
        name, // mangled name
        diFile,
        0, // line no
        diFuncType,
        1, // scope line
        llvm::DISubprogram::FlagPublic,
        llvm::DISubprogram::SPFlagDefinition
    );
    assert(nullptr != funcDefs[name].diFunc);
    fn->setSubprogram(funcDefs[name].diFunc);

    assert(fn->getSubprogram() == funcDefs[name].diFunc);

    return fn;
}

void ModuleBuilder::setCurrentFunc(const std::string &name) {
    assert(funcDefs.find(name) != funcDefs.end());
    funcDefCurrent = name;
    irBuilder.SetInsertPoint(&funcDefs[name].fnPtr->back());
}

Function* ModuleBuilder::getFunc(const std::string &name) {
    if (funcDefs.find(name) == funcDefs.end()) {
        //errs() << "function: " << name << " not created in module\n";
        //assert(false);
        return nullptr;
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
