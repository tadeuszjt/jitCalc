#pragma once

#include <map>
#include <filesystem>

#include "ast.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/DIBuilder.h>

class ModuleBuilder {
public:
    ModuleBuilder(
        llvm::LLVMContext &context,
        const std::string &name,
        const std::filesystem::path &filePath = "jitCalc"
    );

    llvm::Function*       createFuncDeclaration(const std::string&, llvm::Type*, const std::vector<llvm::Type*> &, bool);
    llvm::Function*       createFunc(const std::string &, const std::vector<llvm::Type*> &argTypes, llvm::Type *returnType);
    void                  setCurrentFunc(const std::string &);
    llvm::Argument*       getCurrentFuncArg(size_t argIndex);
    llvm::Function*       getFunc(const std::string &name);

    void                  createGlobalDeclaration(const char*, llvm::Type*);
    llvm::Value*          createCall(size_t line, size_t column, const char *, const std::vector<llvm::Value*> &args);
    llvm::Value*          createInvoke(size_t line, size_t column, llvm::BasicBlock *, llvm::BasicBlock*, const char *, const std::vector<llvm::Value*> &args);
    void                  createTrap();
    llvm::GlobalVariable* getGlobalVariable(const char* name);
    llvm::BasicBlock*     appendNewBlock(const char* suggestion = "block");
    void                  setCurrentBlock(llvm::BasicBlock *);
    llvm::BasicBlock*     getCurrentBlock();

    void              printModule();
    void              optimiseModule();
    void              verifyModule();
    void              finaliseDebug() {
        diBuilder.finalize();
    }

    llvm::Constant* getNullptr() {
        return llvm::ConstantPointerNull::get(irBuilder.getPtrTy());
    }

    llvm::Module &getModule() { return *irModule; }
    std::unique_ptr<llvm::Module> moveModule() { return std::move(irModule); }
    llvm::IRBuilder<> &ir() { return irBuilder; }

private:
    // map stores function data
    struct FuncDef { 
        llvm::Function *fnPtr;
        llvm::DISubprogram *diFunc;
    };
    std::map<std::string, FuncDef> funcDefs;
    std::string                    funcDefCurrent;

    llvm::IRBuilder<>             irBuilder;
    std::unique_ptr<llvm::Module> irModule;

    // debug structures
    llvm::DIBuilder               diBuilder;
    llvm::DIFile                  *diFile;
    llvm::DICompileUnit           *diCompileUnit;
    llvm::DIBasicType             *diInt32Ty;
};
