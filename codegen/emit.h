#pragma once

#include "moduleBuilder.h"
#include "ast.h"
#include "symbols.h"



struct ObjFunc {
    size_t numArgs;
};

struct ObjInt32 {
    llvm::Value *value;
};

struct ObjArg {
    llvm::Value *value;
};

struct ObjVar {
    llvm::AllocaInst *var;
};

using Object = std::variant<ObjFunc, ObjInt32, ObjArg, ObjVar>;
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

    std::vector<std::pair<std::string, int>>& getFuncDefs() {
        return funcDefs;
    }
    void addFuncDefs(std::vector<std::pair<std::string, int>> &funcs) {
        for (const auto &pair : funcs) {
            funcDefs.push_back(pair);
            objTable[symTab.insert(pair.first)] = ObjFunc{.numArgs = pair.second};
        }
    }


    ModuleBuilder &mod() { return builder; }
    SymbolTable &getSymbolTable() { return symTab; }
    std::map<SymbolTable::ID, Object> &getObjTable() { return objTable; }

    Object look(const std::string &name) {
        auto id = symTab.look(name);
        if (objTable.find(id) == objTable.end()) {
            llvm::errs() << name << " not defined. id = " << id << "\n";
            assert(false);
        }
        return objTable[id];
    }
private:

    ModuleBuilder builder;
    SymbolTable   symTab;
    std::map<SymbolTable::ID, Object> objTable;
    std::vector<std::pair<std::string, int>> funcDefs;
};
