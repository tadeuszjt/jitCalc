#pragma once

#include <string>
#include <map>
#include <variant>
#include <vector>

#include <llvm/IR/Instruction.h>



class SymbolTable {
public:
    struct ObjFunc {
        size_t numArgs;
    };

    struct ObjFloat {
        llvm::Value *value;
    };

    struct ObjInt32 {
        llvm::Value *value;
    };

    using Object = std::variant<ObjFunc, ObjFloat, ObjInt32>;


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
