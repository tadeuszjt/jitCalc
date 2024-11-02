#pragma once

#include "moduleBuilder.h"
#include "ast.h"
#include "symbols.h"

class Emit {
public:
    Emit(llvm::LLVMContext &context, const std::string &name);

    void startFunction(const std::string &name);

    void   emitPrint(llvm::Value *value);

    void emitStmt(const ast::Node &);
    void emitFuncDef(const ast::FnDef &);

    llvm::Value* emitExpression(const ast::Node &);
    llvm::Value* emitInfix(const ast::Infix &);
    llvm::Value* emitPrefix(const ast::Prefix &);

    llvm::Value* emitInt32(int n);
    void         emitReturn(llvm::Value *value);
    void         emitReturnNoBlock(llvm::Value *value);

    void emitFuncExtern(const std::string &name, size_t numArgs);

    std::vector<SymbolTable::ObjFunc> getFuncDefs();

    ModuleBuilder &mod() { return builder; };
    SymbolTable &getSymbolTable() { return symTab; };
    void setSymbolTable(SymbolTable &st) { symTab = st; }
private:

    ModuleBuilder builder;
    SymbolTable   symTab;
};
