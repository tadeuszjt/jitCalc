#pragma once

#include "ast.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

class ModuleBuilder {
public:
    ModuleBuilder(llvm::LLVMContext &context, const std::string &name);

    void createExtern(const std::string &name, const std::vector<llvm::Type*> &argTypes, llvm::Type *returnType);
    void setCurrentFunction(const std::string &name);
    llvm::Argument *getCurrentFuncArg(size_t argIndex);
    llvm::Value* createCall(const std::string &name, const std::vector<llvm::Value*> &args);
    llvm::BasicBlock* createFunction(const std::string &name, const std::vector<llvm::Type*> &argTypes, llvm::Type *returnType);

    llvm::BasicBlock* appendNewBlock(const std::string &suggestion = "block");
    void setCurrentBlock(llvm::BasicBlock *);
    llvm::BasicBlock* getCurrentBlock();
    llvm::BasicBlock* getEntryBlock();


    void printModule();
    void optimiseModule();
    void verifyModule();

    llvm::Type *getInt32Ty() { return irBuilder.getInt32Ty(); }
    llvm::Type *getFloatTy() { return irBuilder.getFloatTy(); }
    llvm::Function *getFunction(const std::string &name) { return irModule->getFunction(name); }
    llvm::Module *getModule() { return irModule.get(); }
    std::unique_ptr<llvm::Module> moveModule() { return std::move(irModule); }
    llvm::IRBuilder<> &ir() { return irBuilder; }

private:
    llvm::IRBuilder<>             irBuilder;
    std::unique_ptr<llvm::Module> irModule;
    llvm::Function                *curFn;
};
