#include "moduleBuilder.h"

#include <cmath>




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

Value* ModuleBuilder::createCall(const std::string &name, const std::vector<Value*> &args) {
    return irBuilder.CreateCall(irModule->getFunction(name), args);
}

void ModuleBuilder::createExtern(const std::string &name, Type *returnType) {
    FunctionType *fnType = FunctionType::get(returnType, {}, false);
    assert(fnType != nullptr);
    Function *fn = Function::Create(fnType, Function::ExternalLinkage, name, irModule.get());
    assert(fn != nullptr);
}

void ModuleBuilder::createFunction(const std::string &name, Type *returnType) {
    FunctionType *fnType = FunctionType::get(returnType, {}, false);
    assert(fnType != nullptr);
    Function *fn = Function::Create(fnType, Function::ExternalLinkage, name, irModule.get());
    assert(fn != nullptr);
    BasicBlock *block = BasicBlock::Create(irModule->getContext(), "EntryBlock", fn);
    assert(block != nullptr);
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
