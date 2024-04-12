#pragma once

#include "moduleBuilder.h"
#include "ast.h"

class Emit {
public:
    Emit(LLVMContext &context, const std::string &name);

    void startFunction(const std::string &name);

    void   emitPrint(Value *value);
    Value* emitConvertToFloat(Value *value);

    Value* emitExpression(const ast::Expr &);
    Value* emitInfix(const ast::Infix &);
    Value* emitPrefix(const ast::Prefix &);

    Value* emitInt32(int n);
    Value* emitFloat(float f);
    void   emitReturn(Value *value);

    void emitFuncDef(const ast::FnDef &);

    ModuleBuilder &mod() { return builder; };
private:

    ModuleBuilder builder;
};
