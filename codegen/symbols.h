#pragma once

#include <string>
#include <map>
#include <variant>
#include <vector>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>

class SymbolTable {
public:
    using ID = size_t;

    SymbolTable();
    ID insert(const std::string &symbol);
    ID look(const std::string &symbol);
    bool isDefined(const std::string &symbol);
    void pushScope();
    void popScope();

    std::map<std::string, ID>& getScope(size_t index);

private:
    std::vector<std::map<std::string, ID>> table;
    size_t supply;
};
