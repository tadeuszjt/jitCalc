#pragma once

#include "ast.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>

using namespace llvm;

Value *emitExpression(IRBuilder<> &builder, ast::Expr &expr);
void emitPrint(Module *mod, IRBuilder<> &builder, Value *value);
