#pragma once

#include "ast.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

class ModuleBuilder {
public:
    ModuleBuilder(llvm::LLVMContext &context, const std::string &name);

    void              createFuncDeclaration(const char *, llvm::Type*, const std::vector<llvm::Type*> &, bool);
    void              createGlobalDeclaration(const char*, llvm::Type*);
    llvm::Value*      createCall(const char *, const std::vector<llvm::Value*> &args);
    llvm::BasicBlock* createFunc(const char *, const std::vector<llvm::Type*> &argTypes, llvm::Type *returnType);
    void              createTrap();

    llvm::GlobalVariable* getGlobalVariable(const char* name);

    llvm::BasicBlock* appendNewBlock(const std::string &suggestion = "block");
    void              setCurrentBlock(llvm::BasicBlock *);
    llvm::BasicBlock* getCurrentBlock();
    llvm::BasicBlock* getEntryBlock();
    void              setCurrentFunc(const std::string &name);
    llvm::Argument*   getCurrentFuncArg(size_t argIndex);

    void              printModule();
    void              optimiseModule();
    void              verifyModule();

    llvm::Value* getNullptr() {
        return llvm::ConstantPointerNull::get(irBuilder.getPtrTy());
    }

    llvm::Type *getInt32Ty() { return irBuilder.getInt32Ty(); }
    llvm::Type *getFloatTy() { return irBuilder.getFloatTy(); }
    llvm::Function *getFunc(const std::string &name) { return irModule->getFunction(name); }
    llvm::Module *getModule() { return irModule.get(); }
    std::unique_ptr<llvm::Module> moveModule() { return std::move(irModule); }
    llvm::IRBuilder<> &ir() { return irBuilder; }

private:
    llvm::IRBuilder<>             irBuilder;
    std::unique_ptr<llvm::Module> irModule;
    llvm::Function                *curFn;
};
