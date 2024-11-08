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


void Emit::emitStmt(const ast::Node &stmt) {
    if (auto *fnDef = dyn_cast<ast::FnDef>(&stmt)) {
        emitFuncDef(*fnDef);
        return;
    }
    if (auto *ret = dyn_cast<ast::Return>(&stmt)) {
        auto *e = emitExpression(*ret->expr);
        emitReturn(e);
        return;
    }
    if (auto *if_ = dyn_cast<ast::If>(&stmt)) {
        auto *cnd = emitExpression(*if_->cnd);
        auto *cmp = builder.ir().CreateICmpEQ(cnd, emitInt32(0));

        BasicBlock *True = builder.appendNewBlock();
        BasicBlock *end = builder.appendNewBlock();

        builder.ir().CreateCondBr(cmp, end, True);

        builder.setCurrentBlock(True);
        symTab.pushScope();
        for (ast::Node *stmtPtr : (*if_->body).list) {
            emitStmt(*stmtPtr);
        }
        symTab.popScope();
        builder.ir().CreateBr(end);

        builder.setCurrentBlock(end);
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
        std::cout << "defining: " << (*fnDef.args).list[i]->ident << std::endl;
        symTab.insert((*fnDef.args).list[i]->ident,
                      SymbolTable::ObjFloat{builder.getCurrentFuncArg(i)});
    }

    for (ast::Node *stmtPtr : (*fnDef.body).list) {
        emitStmt(*stmtPtr);
    }


    symTab.popScope();

    emitReturnNoBlock(emitInt32(0));
}


Value* Emit::emitInt32(int n) {
    return builder.ir().getInt32(n);
}


void Emit::startFunction(const std::string &name) {
    builder.createFunction(name, {}, builder.getInt32Ty());
    builder.setCurrentFunction(name);
}

void Emit::emitReturn(Value *value) {
    BasicBlock *emptyBlock = builder.appendNewBlock();
    builder.ir().CreateRet(value);
    builder.setCurrentBlock(emptyBlock);
}

void Emit::emitReturnNoBlock(Value *value) {
    builder.ir().CreateRet(value);
}

Value* Emit::emitExpression(const ast::Node &expr) {
    if (auto *integer = dyn_cast<ast::Integer>(&expr)) {
        return emitInt32(integer->integer);
    }
    if (auto *floating = dyn_cast<ast::Floating>(&expr)) {
        assert(false);
    }
    if (auto *infix = dyn_cast<ast::Infix>(&expr)) {
        return emitInfix(*infix);
    }
    if (auto *prefix = dyn_cast<ast::Prefix>(&expr)) {
        return emitPrefix(*prefix);
    }
    if (auto *ident = dyn_cast<ast::Ident>(&expr)) {
        auto object = symTab.look(ident->ident);
        if (std::holds_alternative<SymbolTable::ObjFloat>(object)) {
            return std::get<SymbolTable::ObjFloat>(object).value;
        }

        assert(false);
    }
    if (auto *call = dyn_cast<ast::Call>(&expr)) {
        auto object = symTab.look(call->name);
        if (std::holds_alternative<SymbolTable::ObjFunc>(object)) {
            size_t num_args = std::get<SymbolTable::ObjFunc>(object).numArgs;
            assert(call->args->size() == num_args);

            std::vector<Value*> vals;
            for (auto exprPtr : (*call->args).list) {
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

    switch (infix.op) { 
    case ast::Plus: return builder.ir().CreateAdd(left, right,  "infix");
    case ast::Minus: return builder.ir().CreateSub(left, right,  "infix");
    case ast::Times: return builder.ir().CreateMul(left, right,  "infix");
    case ast::Divide: return builder.ir().CreateSDiv(left, right, "infix");
    case ast::LT: {
        auto *cmp = builder.ir().CreateICmpSLT(left, right);
        auto *i32 = builder.ir().CreateZExt(cmp, builder.getInt32Ty());
        return i32;
    }
    case ast::GT: {
        auto *cmp = builder.ir().CreateICmpSGT(left, right);
        auto *i32 = builder.ir().CreateZExt(cmp, builder.getInt32Ty());
        return i32;
    }
    case ast::EqEq: {
        auto *cmp = builder.ir().CreateICmpEQ(left, right);
        auto *i32 = builder.ir().CreateZExt(cmp, builder.getInt32Ty());
        return i32;
    }
    default: assert(false); break;
    }

    assert(false);
    return nullptr;
}

Value* Emit::emitPrefix(const ast::Prefix &prefix) {
    auto *right = emitExpression(*prefix.right);

    if (right->getType() == builder.getInt32Ty()) {
        if (prefix.op == ast::Minus) {
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
        Value *format = builder.ir().CreateGlobalString("%d\n");
        printfArgs = {format, value};
    } else {
        assert(false);
    }

    builder.createCall("printf", printfArgs);
}
