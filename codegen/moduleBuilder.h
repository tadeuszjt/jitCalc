#pragma once

#include <map>
#include <filesystem>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/DIBuilder.h>

#include "ast.h"
#include "sparse.h"

class ModuleBuilder {
public:
    struct VarLocal {
        llvm::DILocalVariable *diLocalVar;
    };


    ModuleBuilder(
        llvm::LLVMContext &context,
        const std::string &name,
        const std::filesystem::path &filePath = "jitCalc"
    );

    llvm::Function*       createFuncDeclaration(const std::string&, llvm::Type*, const std::vector<llvm::Type*> &, bool);
    llvm::Function*       createFunc(TextPos pos, const std::string &, const std::vector<llvm::Type*> &argTypes, llvm::Type *returnType);
    void                  setCurrentFunc(const std::string &);
    llvm::Argument*       getCurrentFuncArg(size_t argIndex);
    llvm::Function*       getFunc(const std::string &name);

    void                  createGlobalDeclaration(const char*, llvm::Type*);
    llvm::Value*          createCall(const char *, const std::vector<llvm::Value*> &args);
    llvm::Value*          createCall(TextPos pos, const char *, const std::vector<llvm::Value*> &args);
    llvm::Value*          createInvoke(TextPos pos, llvm::BasicBlock *, llvm::BasicBlock*, const char *, const std::vector<llvm::Value*> &args);
    void                  createTrap();
    llvm::GlobalVariable* getGlobalVariable(const char* name);
    llvm::BasicBlock*     appendNewBlock(const char* suggestion = "block");
    void                  setCurrentBlock(llvm::BasicBlock *);
    llvm::BasicBlock*     getCurrentBlock();

    Sparse<VarLocal>::Key createVarLocalDebug(const char *name);
    Sparse<VarLocal>::Key createArgDebug(const char *name, int argIdx);
    void                  setVarLocalDebugValue(TextPos pos, Sparse<VarLocal>::Key key, llvm::Value *value);


    void              printModule();
    void              optimiseModule();
    void              verifyModule();
    void              finaliseDebug() {
        diBuilder.finalize();
    }

    llvm::Constant* getNullptr() {
        return llvm::ConstantPointerNull::get(irBuilder.getPtrTy());
    }

    llvm::Module &getLlModule() { return *llModule; }
    std::unique_ptr<llvm::Module> moveModule() { return std::move(llModule); }
    llvm::IRBuilder<> &ir() { return irBuilder; }

private:
    // map stores function data
    struct FuncDef { 
        llvm::Function *fnPtr;
        llvm::DISubprogram *diFunc;
        llvm::DILocalScope *diScope;
    };
    std::map<std::string, FuncDef> funcDefs;
    std::string                    funcDefCurrent;


    llvm::IRBuilder<>             irBuilder;
    std::unique_ptr<llvm::Module> llModule;

    // debug structures
    llvm::DIBuilder               diBuilder;
    llvm::DIFile                  *diFile;
    llvm::DICompileUnit           *diCompileUnit;
    llvm::DIBasicType             *diInt32Ty;


    // variable debug
    Sparse<VarLocal> varLocals;


};
