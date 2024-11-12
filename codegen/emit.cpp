#include "emit.h"

#include <cmath>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>

using namespace llvm;


Emit::Emit(LLVMContext &context, const std::string &name)
    : builder(context, name) {
}


void Emit::emitFuncExtern(const std::string &name, size_t numArgs) {
    objTable[symTab.insert(name)] = ObjFunc{numArgs};
    std::vector<Type*> argTypes(numArgs, builder.ir().getInt32Ty());
    builder.createExtern(name, argTypes, builder.ir().getInt32Ty());
}


void Emit::emitStmt(const ast::Node &stmt) {
    if (auto *fnDef = dyn_cast<ast::FnDef>(&stmt)) {
        emitFuncDef(*fnDef);
    } else if (auto *ret = dyn_cast<ast::Return>(&stmt)) {
        auto *e = emitExpression(*ret->expr);
        emitReturn(e);
    } else if (auto *let = dyn_cast<ast::Let>(&stmt)) {
        auto *expr = emitExpression(*let->expr);

        auto *curBlk = builder.getCurrentBlock();
        builder.setCurrentBlock(builder.getEntryBlock());
        llvm::AllocaInst *var = builder.ir().CreateAlloca(builder.ir().getInt32Ty());
        objTable[symTab.insert(let->name)] = ObjVar{var};
        builder.setCurrentBlock(curBlk);

        builder.ir().CreateStore(expr, var);
    } else if (auto *set = dyn_cast<ast::Set>(&stmt)) {
        auto *expr = emitExpression(*set->expr);
        auto obj = look(set->name);
        assert(std::holds_alternative<ObjVar>(obj));
        builder.ir().CreateStore(expr, std::get<ObjVar>(obj).var);
    } else if (auto *if_ = dyn_cast<ast::If>(&stmt)) {
        auto *cnd = emitExpression(*if_->cnd);
        auto *cmp = builder.ir().CreateICmpEQ(cnd, emitInt32(0));

        BasicBlock *trueBlk = builder.appendNewBlock();
        BasicBlock *falseBlk = builder.appendNewBlock();
        BasicBlock *end = builder.appendNewBlock();

        builder.ir().CreateCondBr(cmp, falseBlk, trueBlk);

        builder.setCurrentBlock(trueBlk);
        symTab.pushScope();
        for (ast::Node *stmtPtr : (*if_->trueBody).list) {
            emitStmt(*stmtPtr);
        }
        symTab.popScope();
        builder.ir().CreateBr(end);

        builder.setCurrentBlock(falseBlk);
        symTab.pushScope();
        for (ast::Node *stmtPtr : (*if_->falseBody).list) {
            emitStmt(*stmtPtr);
        }
        symTab.popScope();
        builder.ir().CreateBr(end);

        builder.setCurrentBlock(end);
    } else if (auto *for_ = dyn_cast<ast::For>(&stmt)) {
        BasicBlock *curBlk = builder.getCurrentBlock();
        BasicBlock *forBlk = builder.appendNewBlock("for");
        BasicBlock *bdyBlk = builder.appendNewBlock("body");
        BasicBlock *bdyEndBlk = builder.appendNewBlock("bodyEnd");
        BasicBlock *endBlk = builder.appendNewBlock("end");

        builder.ir().CreateBr(forBlk);
        builder.setCurrentBlock(forBlk);

        PHINode *idx = builder.ir().CreatePHI(builder.getInt32Ty(), 2);
        idx->addIncoming(builder.ir().getInt32(0), curBlk);
        auto *idx2 = builder.ir().CreateAdd(idx, builder.ir().getInt32(1),  "increment");
        idx->addIncoming(idx2, bdyEndBlk);

        auto *val = emitExpression(*for_->cnd);
        auto *cnd = builder.ir().CreateICmpSLT(idx, val);

        builder.ir().CreateCondBr(cnd, bdyBlk, endBlk);

        builder.setCurrentBlock(bdyBlk);
        symTab.pushScope();
        for (ast::Node *stmtPtr : (*for_->body).list) {
            emitStmt(*stmtPtr);
        }
        symTab.popScope();
        builder.ir().CreateBr(bdyEndBlk);
        builder.setCurrentBlock(bdyEndBlk);
        builder.ir().CreateBr(forBlk);

        builder.setCurrentBlock(endBlk);
    } else {
        assert(false);
    }
}


void Emit::emitFuncDef(const ast::FnDef& fnDef) {
    objTable[symTab.insert(fnDef.name->ident)] = ObjFunc{fnDef.args->size()};
    funcDefs.push_back(std::make_pair<std::string, int>(std::string(fnDef.name->ident), fnDef.args->size()));

    std::vector<Type*> argTypes(fnDef.args->size(), builder.ir().getInt32Ty());
    llvm::BasicBlock *entry = builder.createFunction(
        fnDef.name->ident, argTypes, builder.ir().getInt32Ty());
    builder.setCurrentFunction(fnDef.name->ident);

    symTab.pushScope();

    for (int i = 0; i < fnDef.args->size(); i++) {
        std::cout << "defining: " << (*fnDef.args).list[i]->ident << std::endl;
        objTable[symTab.insert((*fnDef.args).list[i]->ident)] = ObjArg{builder.getCurrentFuncArg(i)};
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
        auto object = look(ident->ident);

        if (std::holds_alternative<ObjArg>(object)) {
            return std::get<ObjArg>(object).value;
        }
        if (std::holds_alternative<ObjVar>(object)) {
            return builder.ir().CreateLoad(
                builder.ir().getInt32Ty(), std::get<ObjVar>(object).var);
        }

        assert(false);
    }
    if (auto *call = dyn_cast<ast::Call>(&expr)) {
        auto object = look(call->name);
        if (std::holds_alternative<ObjFunc>(object)) {
            size_t num_args = std::get<ObjFunc>(object).numArgs;
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
