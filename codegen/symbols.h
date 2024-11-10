#pragma once

#include <string>
#include <map>
#include <variant>
#include <vector>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>



class SymbolTable {
public:
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


    SymbolTable();
    void insert(const std::string &symbol, const Object &object);
    Object look(const std::string &symbol);
    void pushScope();
    void popScope();

    SymbolTable(SymbolTable &symTab) : table(symTab.table) {}
    void operator=(SymbolTable &symTab) { table = symTab.table; }

private:
    std::vector<std::map<std::string, Object>> table;
};
