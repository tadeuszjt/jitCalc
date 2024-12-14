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
    , llModule(std::make_unique<Module>(name, context))
    , funcDefCurrent("")
    , diBuilder(*llModule)
{
    llModule->setSourceFileName(debugFilePath.filename().c_str());


    // create debug structures
    diFile = diBuilder.createFile(
        debugFilePath.filename().c_str(),
        debugFilePath.parent_path().c_str()
    );
    assert(nullptr != diFile);

    diCompileUnit = diBuilder.createCompileUnit(
        llvm::dwarf::DW_LANG_C,
        diFile,
        "jitCalc",
        false,
        "",
        0
    );
    assert(nullptr != diCompileUnit);

    llModule->addModuleFlag(llvm::Module::Warning, "Debug Info Version", llvm::DEBUG_METADATA_VERSION);
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
    MPM.run(*llModule, MAM);
}


void ModuleBuilder::createGlobalDeclaration(const char* name, llvm::Type* type) {
    auto *globalVar = new GlobalVariable(
        *llModule,
        type,
        false,
        GlobalValue::ExternalLinkage,
        nullptr,
        name);
    assert(globalVar != nullptr);
}

llvm::GlobalVariable* ModuleBuilder::getGlobalVariable(const char* name) {
    return llModule->getGlobalVariable(name);
}


Value* ModuleBuilder::createCall(const char *name, const std::vector<Value*> &args) {
    assert(funcDefs.find(name) != funcDefs.end());
    return irBuilder.CreateCall(llModule->getFunction(name), args);
}

Value* ModuleBuilder::createCall(TextPos pos, const char *name, const std::vector<Value*> &args) {
    assert(funcDefs.find(name) != funcDefs.end());
    auto *call = irBuilder.CreateCall(llModule->getFunction(name), args);

    if (nullptr != funcDefs[funcDefCurrent].diFunc) {
        call->setDebugLoc(llvm::DILocation::get(
            llModule->getContext(), pos.line, pos.column, funcDefs[funcDefCurrent].diFunc));
    }

    return call;
}

llvm::Value* ModuleBuilder::createInvoke(
    TextPos pos,
    llvm::BasicBlock *normalDest,
    llvm::BasicBlock* unwindDest,
    const char *name,
    const std::vector<llvm::Value*> &args)
{
    auto *invoke = irBuilder.CreateInvoke(llModule->getFunction(name), normalDest, unwindDest, args);

    if (nullptr != funcDefs[funcDefCurrent].diFunc) {
        auto *diLoc = llvm::DILocation::get(
            llModule->getContext(), pos.line, pos.column, funcDefs[funcDefCurrent].diFunc);
        invoke->setDebugLoc(diLoc);
    }

    return invoke;
}


void ModuleBuilder::createTrap() {
    Function *trap = Intrinsic::getOrInsertDeclaration(llModule.get(), Intrinsic::trap);
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
        llModule.get()
    );
    assert(nullptr != fn);
    funcDefs[name].fnPtr = fn;
    funcDefs[name].diFunc = nullptr;
    funcDefs[name].diScope = nullptr;
    
    return fn;
}


Function* ModuleBuilder::createFunc(
    TextPos pos,
    const std::string &name,
    const std::vector<Type*> &argTypes,
    Type *returnType
) {
    auto *fn = createFuncDeclaration(name, returnType, argTypes, false);
    BasicBlock *block = BasicBlock::Create(llModule->getContext(), "EntryBlock", fn);
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
        pos.line, // line no
        diFuncType,
        pos.line, // scope line
        llvm::DISubprogram::FlagPublic,
        llvm::DISubprogram::SPFlagDefinition
    );
    assert(nullptr != funcDefs[name].diFunc);
    fn->setSubprogram(funcDefs[name].diFunc);

    funcDefs[name].diScope = diBuilder.createLexicalBlock(
        funcDefs[name].diFunc,
        diFile,
        pos.line, // line no
        pos.column  // col no
    ); 

    assert(nullptr != funcDefs[name].diScope);
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

Sparse<ModuleBuilder::VarLocal>::Key ModuleBuilder::createVarLocalDebug(const char *name) {
    auto *fn = funcDefs[funcDefCurrent].diFunc;
    assert(nullptr != fn);
    auto *var = diBuilder.createAutoVariable(
        fn, name, diFile, 2, // line number
        diBuilder.createBasicType("INTEGER", 32, llvm::dwarf::DW_ATE_signed));
    assert(nullptr != var);

    auto key = varLocals.insert();
    varLocals.at(key).diLocalVar = var;
    return key;
}

Sparse<ModuleBuilder::VarLocal>::Key ModuleBuilder::createArgDebug(const char *name, int argIdx) {
    auto *fn = funcDefs[funcDefCurrent].diFunc;
    auto *var = diBuilder.createParameterVariable(
        fn, name, argIdx, diFile, 2, // line number
        diBuilder.createBasicType("INTEGER", 32, llvm::dwarf::DW_ATE_signed));
    assert(nullptr != var);

    auto key = varLocals.insert();
    varLocals.at(key).diLocalVar = var;
    return key;
}


void ModuleBuilder::setVarLocalDebugValue(
    TextPos pos,
    Sparse<ModuleBuilder::VarLocal>::Key key,
    llvm::Value *value
) {
    auto *diLoc = llvm::DILocation::get(
        llModule->getContext(), pos.line, pos.column, funcDefs[funcDefCurrent].diFunc);
    diBuilder.insertDbgValueIntrinsic(
        value,
        varLocals.at(key).diLocalVar,
        diBuilder.createExpression(),
        diLoc,
        getCurrentBlock()
    );
}


void ModuleBuilder::printModule() {
    llModule->print(outs(), nullptr);
}

void ModuleBuilder::verifyModule() {
    bool failed = llvm::verifyModule(*llModule, &llvm::errs());
    assert(!failed);
}
