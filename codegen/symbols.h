#pragma once

#include <string>
#include <map>


class SymbolTable {
public:
    void addFunction(const std::string &name, int numArgs);
    bool hasFunction(const std::string &name);
    int getFunction(const std::string &name);

private:
    std::map<std::string, int> functionMap;
};
