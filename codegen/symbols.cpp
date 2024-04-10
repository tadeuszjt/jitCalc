#include "symbols.h"

#include <cassert>


void SymbolTable::addFunction(const std::string &name, int numArgs) {
    assert(functionMap.find(name) == functionMap.end());
    functionMap[name] = numArgs;
}

bool SymbolTable::hasFunction(const std::string &name) {
    return functionMap.find(name) != functionMap.end();
}

int SymbolTable::getFunction(const std::string &name) {
    assert(hasFunction(name));
    return functionMap[name];
}
