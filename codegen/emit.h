#pragma once

#include "moduleBuilder.h"
#include "ast.h"

class Emit {
public:
    Emit(LLVMContext &context, const std::string &name);
private:

    ModuleBuilder builder;
};
