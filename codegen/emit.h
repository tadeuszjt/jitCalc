#pragma once

#include "moduleBuilder.h"
#include "ast.h"
#include "symbols.h"

#include <set>
#include <map>
#include <vector>

struct ObjFunc {
    size_t numArgs;
};

struct ObjVar {
};

using Object = std::variant<ObjFunc, ObjVar>;
class Emit {
public:
    Emit(llvm::LLVMContext &context, const std::string &name);

    void startFunction(const std::string &name);
    void printf(const char* fmt, std::vector<llvm::Value*> args);

    void         emitStmt(const ast::Node &);
    void         emitFuncDef(const ast::FnDef &);
    llvm::Value* emitExpression(const ast::Node &);
    llvm::Value* emitInfix(const ast::Infix &);
    llvm::Value* emitPrefix(const ast::Prefix &);
    llvm::Value* emitInt32(int n);
    llvm::Value* emitCall(const ast::Call&, bool);
    void         emitReturn(llvm::Value *value);
    void         emitReturnNoBlock(llvm::Value *value);
    void         emitFuncExtern(const std::string &name, size_t numArgs);

    std::vector<std::pair<std::string, ObjFunc>>& getFuncDefs() {
        return funcDefs;
    }
    void addFuncDefs(std::vector<std::pair<std::string, ObjFunc>> &funcs) {
        for (const auto &pair : funcs) {
            funcDefs.push_back(pair);
            objTable[symTab.insert(pair.first)] = pair.second;
        }
    }

    ModuleBuilder &mod() { return builder; }
private:
    Object look(const std::string &name);
    void define(const std::string &name, Object object);
    void redefine(const std::string &name, Object object);

    ModuleBuilder builder;
    SymbolTable   symTab;
    std::map<SymbolTable::ID, Object> objTable;
    std::vector<std::pair<std::string, ObjFunc>> funcDefs;


    // AST Numbering algorithm for Phi-node generation
    void          writeVariable(SymbolTable::ID, llvm::BasicBlock*, llvm::Value *);
    llvm::Value*  readVariable(SymbolTable::ID variable, llvm::BasicBlock* block);
    llvm::Value*  readVariableRecursive(SymbolTable::ID, llvm::BasicBlock* );
    void          sealBlock(llvm::BasicBlock *);
    llvm::PHINode *newPhi(llvm::BasicBlock* );
    void          addPhiOperands(SymbolTable::ID, llvm::PHINode *);

    std::map<SymbolTable::ID, std::map<llvm::BasicBlock*, llvm::Value*>> currentDefs;
    std::map<llvm::BasicBlock*, std::map<SymbolTable::ID, llvm::PHINode*>> incompletePhis;
    std::set<llvm::BasicBlock*> sealedBlocks;
};
