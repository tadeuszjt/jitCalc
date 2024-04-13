#include "emit.h"

#include <cmath>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>

using namespace llvm;


Emit::Emit(LLVMContext &context, const std::string &name)
    : builder(context, name) {
}


void Emit::emitFuncExtern(const std::string &name, size_t numArgs) {
    symTab.insert(name, SymbolTable::ObjFunc{numArgs});
    std::vector<Type*> argTypes(numArgs, builder.ir().getInt32Ty());
    builder.createExtern(name, argTypes, builder.ir().getInt32Ty());
}


void Emit::emitStmt(const ast::Stmt &stmt) {
    if (auto *fnDef = dynamic_cast<const ast::FnDef*>(&stmt)) {
        emitFuncDef(*fnDef);
        return;
    }
    if (auto *ret = dynamic_cast<const ast::Return*>(&stmt)) {
        auto *e = emitExpression(*ret->expr);
        emitReturn(e);
        return;
    }

    assert(false);
}

void Emit::emitFuncDef(const ast::FnDef& fnDef) {
    symTab.insert(fnDef.name->ident, SymbolTable::ObjFunc{fnDef.args->size()});

    std::vector<Type*> argTypes(fnDef.args->size(), builder.ir().getInt32Ty());
    builder.createFunction(fnDef.name->ident, argTypes, builder.ir().getInt32Ty());
    builder.setCurrentFunction(fnDef.name->ident);

    symTab.pushScope();

    for (int i = 0; i < fnDef.args->size(); i++) {
        std::cout << "defining: " << (*fnDef.args).identList[i]->ident << std::endl;
        symTab.insert((*fnDef.args).identList[i]->ident,
                      SymbolTable::ObjFloat{builder.getCurrentFuncArg(i)});
    }

    for (std::shared_ptr<ast::Stmt> &stmtPtr : (*fnDef.body).stmtList) {
        emitStmt(*stmtPtr);
    }


    symTab.popScope();
}


Value* Emit::emitInt32(int n) {
    return builder.ir().getInt32(n);
}


void Emit::startFunction(const std::string &name) {
    builder.createFunction(name, {}, builder.getInt32Ty());
    builder.setCurrentFunction(name);
}

void Emit::emitReturn(Value *value) {
    builder.ir().CreateRet(value);
}


Value* Emit::emitExpression(const ast::Expr &expr) {
    if (auto *integer = dynamic_cast<const ast::Integer*>(&expr)) {
        return emitInt32(integer->integer);
    }
    if (auto *floating = dynamic_cast<const ast::Floating*>(&expr)) {
        assert(false);
    }
    if (auto *infix = dynamic_cast<const ast::Infix*>(&expr)) {
        return emitInfix(*infix);
    }
    if (auto *prefix = dynamic_cast<const ast::Prefix*>(&expr)) {
        return emitPrefix(*prefix);
    }
    if (auto *ident = dynamic_cast<const ast::Ident*>(&expr)) {
        auto object = symTab.look(ident->ident);
        if (std::holds_alternative<SymbolTable::ObjFloat>(object)) {
            return std::get<SymbolTable::ObjFloat>(object).value;
        }

        assert(false);
    }
    if (auto *call = dynamic_cast<const ast::Call*>(&expr)) {
        auto object = symTab.look(call->name);
        if (std::holds_alternative<SymbolTable::ObjFunc>(object)) {
            size_t num_args = std::get<SymbolTable::ObjFunc>(object).numArgs;
            assert(call->args->size() == num_args);

            std::vector<Value*> vals;
            for (auto exprPtr : (*call->args).exprList) {
                vals.push_back(emitExpression(*exprPtr));
            }

            std::vector<Type*> argTypes(num_args, builder.ir().getInt32Ty());
            builder.createExtern(call->name, argTypes, builder.getInt32Ty());
            return builder.createCall(call->name, vals);
        }
    }
    assert(false);
    return nullptr;
}

Value* Emit::emitInfix(const ast::Infix &infix) {
    auto *left = emitExpression(*infix.left);
    auto *right = emitExpression(*infix.right);

    switch (infix.symbol.symbol) { 
    case '+': return builder.ir().CreateAdd(left, right,  "infix");
    case '-': return builder.ir().CreateSub(left, right,  "infix");
    case '*': return builder.ir().CreateMul(left, right,  "infix");
    case '/': return builder.ir().CreateSDiv(left, right, "infix");
    default: assert(false); break;
    }

    assert(false);
    return nullptr;
}

Value* Emit::emitPrefix(const ast::Prefix &prefix) {
    auto *right = emitExpression(*prefix.right);

    if (right->getType() == builder.getInt32Ty()) {
        if (prefix.symbol.symbol == '-') {
            return builder.ir().CreateSub(emitInt32(0), right, "prefix");
        }
        assert(false);
    }
    
    assert(false);
    return nullptr;
}

void Emit::emitPrint(Value *value) {
    std::vector<Value*> printfArgs;

    if (value->getType() == builder.ir().getInt32Ty()) {
        Value *format = builder.ir().CreateGlobalStringPtr("%d\n");
        printfArgs = {format, value};
    } else {
        assert(false);
    }

    builder.createCall("printf", printfArgs);
}
