#include "emit.h"

#include <cmath>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>


Emit::Emit(LLVMContext &context, const std::string &name)
    : builder(context, name) {
}


void Emit::emitFuncDef(const ast::FnDef& fnDef) {
    builder.createFunction(fnDef.name->ident, builder.ir().getFloatTy());
    builder.setCurrentFunction(fnDef.name->ident);

    auto *v1 = emitExpression(*fnDef.body);
    auto *v2 = emitConvertToFloat(v1);
    emitReturn(v2);
}


Value* Emit::emitInt32(int n) {
    return builder.ir().getInt32(n);
}

Value* Emit::emitFloat(float f) {
    Constant *cf = ConstantFP::get(builder.getFloatTy(), f);
    return builder.ir().CreateFPCast(cf, builder.getFloatTy());
}

void Emit::startFunction(const std::string &name) {
    builder.createFunction(name, builder.getInt32Ty());
    builder.setCurrentFunction(name);
}

void Emit::emitReturn(Value *value) {
    builder.ir().CreateRet(value);
}

Value* Emit::emitConvertToFloat(Value *value) {
    if (value->getType() == builder.getInt32Ty()) {
        return builder.ir().CreateSIToFP(value, builder.getFloatTy());
    } else if (value->getType() == builder.getFloatTy()) {
        return value;
    } else {
        assert(false);
    }
}

Value* Emit::emitExpression(const ast::Expr &expr) {
    if (auto *integer = dynamic_cast<const ast::Integer*>(&expr)) {
        return emitInt32(integer->integer);
    }
    if (auto *floating = dynamic_cast<const ast::Floating*>(&expr)) {
        return emitFloat(floating->floating);
    }
    if (auto *infix = dynamic_cast<const ast::Infix*>(&expr)) {
        return emitInfix(*infix);
    }
    if (auto *prefix = dynamic_cast<const ast::Prefix*>(&expr)) {
        return emitPrefix(*prefix);
    }
    if (auto *call = dynamic_cast<const ast::Call*>(&expr)) {
        if (call->name == "pi") {
            return emitFloat(M_PI);
        } else {
            builder.createExtern(call->name, builder.getFloatTy());
            return builder.createCall(call->name, {});
        }
    }
    assert(false);
    return nullptr;
}

Value* Emit::emitInfix(const ast::Infix &infix) {
    auto *left = emitExpression(*infix.left);
    auto *right = emitExpression(*infix.right);


    if (right->getType() == builder.getInt32Ty() && left->getType() == builder.getInt32Ty()) {
        switch (infix.symbol.symbol) { 
        case '+': return builder.ir().CreateAdd(left, right,  "infix");
        case '-': return builder.ir().CreateSub(left, right,  "infix");
        case '*': return builder.ir().CreateMul(left, right,  "infix");
        case '/': return builder.ir().CreateSDiv(left, right, "infix");
        default: assert(false); break;
        }
    }

    if (right->getType() == builder.getFloatTy() && left->getType() == builder.getFloatTy()) {
        switch (infix.symbol.symbol) { 
        case '+': return builder.ir().CreateFAdd(left, right,  "infix");
        case '-': return builder.ir().CreateFSub(left, right,  "infix");
        case '*': return builder.ir().CreateFMul(left, right,  "infix");
        case '/': return builder.ir().CreateFDiv(left, right, "infix");
        default: assert(false); break;
        }
    }


    if (
        (right->getType() == builder.getInt32Ty() && left->getType() == builder.getFloatTy()) ||
        (right->getType() == builder.getFloatTy() && left->getType() == builder.getInt32Ty())
    ) {
        Value *rightFloat = right;
        Value *leftFloat = left;

        if (right->getType() == builder.getInt32Ty()) {
            rightFloat = builder.ir().CreateSIToFP(right, builder.getFloatTy());
        }
        if (left->getType() == builder.getInt32Ty()) {
            leftFloat = builder.ir().CreateSIToFP(left, builder.getFloatTy());
        }

        switch (infix.symbol.symbol) { 
        case '+': return builder.ir().CreateFAdd(leftFloat, rightFloat,  "infix");
        case '-': return builder.ir().CreateFSub(leftFloat, rightFloat,  "infix");
        case '*': return builder.ir().CreateFMul(leftFloat, rightFloat,  "infix");
        case '/': return builder.ir().CreateFDiv(leftFloat, rightFloat, "infix");
        default: assert(false); break;
        }
    }

    assert(false);
    return nullptr;
}

Value* Emit::emitPrefix(const ast::Prefix &prefix) {
    auto *right = emitExpression(*prefix.right);

    // Value is integer
    if (right->getType() == builder.getInt32Ty()) {
        if (prefix.symbol.symbol == '-') {
            return builder.ir().CreateSub(emitInt32(0), right, "prefix");
        }
        assert(false);
    }
    

    // Value is float
    if (right->getType() == builder.getFloatTy()) {
        Constant *zeroConst = ConstantFP::get(builder.getFloatTy(), 0.0);
        if (prefix.symbol.symbol == '-') {
            return builder.ir().CreateFSub(zeroConst, right, "prefix");
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
    }
    else if (value->getType() == builder.getFloatTy()) {
        Value *format = builder.ir().CreateGlobalStringPtr("%f\n");

        Value *doub = builder.ir().CreateFPExt(value, builder.ir().getDoubleTy());
        printfArgs = {format, doub};
    } else {
        assert(false);
    }

    builder.createCall("printf", printfArgs);
}
