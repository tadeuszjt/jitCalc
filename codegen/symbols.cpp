#include "symbols.h"

#include <cassert>
#include <iostream>

SymbolTable::SymbolTable() {
    // enure first scope level exists
    pushScope();
}

SymbolTable::ID SymbolTable::insert(const std::string &symbol) {
    assert(table.back().find(symbol) == table.back().end());
    SymbolTable::ID id = supply++;
    table.back()[symbol] = id;
    return id;
}

SymbolTable::ID SymbolTable::look(const std::string &symbol) {
    for (auto it = table.rbegin(); it != table.rend(); it++) {
        if (it->find(symbol) != it->end()) {
            return (*it)[symbol];
        }
    }

    std::cerr << "Error: symbol not defined: " << symbol << std::endl;
    assert(false);
}


void SymbolTable::pushScope() {
    table.emplace_back();
}


void SymbolTable::popScope() {
    assert(table.size() > 1);
    table.pop_back();
}
