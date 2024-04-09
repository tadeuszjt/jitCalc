#include "emit.h"

#include <cmath>


void emitPrint(Module *mod, IRBuilder<> &builder, Value *value) {
    if (value->getType() == builder.getInt32Ty()) {
        Value *format = builder.CreateGlobalStringPtr("benis: %d\n");
        std::vector<Value*> args = {format, value};
        builder.CreateCall(mod->getFunction("printf"), args);
        return;
    }

    if (value->getType() == builder.getFloatTy()) {
        Value *format = builder.CreateGlobalStringPtr("benis: %f\n");

        Value *doub = builder.CreateFPExt(value, builder.getDoubleTy());
        std::vector<Value*> args = {format, doub};
        builder.CreateCall(mod->getFunction("printf"), args);
        return;
    }

    assert(false);
}

Value *emitInfix(IRBuilder<> &builder, const ast::Infix &infix) {
    auto *left = emitExpression(builder, *infix.left);
    auto *right = emitExpression(builder, *infix.right);


    if (right->getType() == builder.getInt32Ty() && left->getType() == builder.getInt32Ty()) {
        switch (infix.symbol) { 
        case '+': return builder.CreateAdd(left, right,  "infix");
        case '-': return builder.CreateSub(left, right,  "infix");
        case '*': return builder.CreateMul(left, right,  "infix");
        case '/': return builder.CreateSDiv(left, right, "infix");
        default: assert(false); break;
        }
    }

    if (right->getType() == builder.getFloatTy() && left->getType() == builder.getFloatTy()) {
        switch (infix.symbol) { 
        case '+': return builder.CreateFAdd(left, right,  "infix");
        case '-': return builder.CreateFSub(left, right,  "infix");
        case '*': return builder.CreateFMul(left, right,  "infix");
        case '/': return builder.CreateFDiv(left, right, "infix");
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
            rightFloat = builder.CreateSIToFP(right, builder.getFloatTy());
        }
        if (left->getType() == builder.getInt32Ty()) {
            leftFloat = builder.CreateSIToFP(left, builder.getFloatTy());
        }

        switch (infix.symbol) { 
        case '+': return builder.CreateFAdd(leftFloat, rightFloat,  "infix");
        case '-': return builder.CreateFSub(leftFloat, rightFloat,  "infix");
        case '*': return builder.CreateFMul(leftFloat, rightFloat,  "infix");
        case '/': return builder.CreateFDiv(leftFloat, rightFloat, "infix");
        default: assert(false); break;
        }
    }



    assert(false);
    return nullptr;
}

Value *emitPrefix(IRBuilder<> &builder, const ast::Prefix &prefix) {
    auto *right = emitExpression(builder, *prefix.right);

    // Value is integer
    if (right->getType() == builder.getInt32Ty()) {
        if (prefix.symbol == '-') {
            return builder.CreateSub(builder.getInt32(0), right, "prefix");
        }
        assert(false);
    }

    // Value is float
    if (right->getType() == builder.getFloatTy()) {
        Constant *zeroConst = ConstantFP::get(builder.getFloatTy(), 0.0);
        if (prefix.symbol == '-') {
            return builder.CreateFSub(zeroConst, right, "prefix");
        }
        assert(false);
    }

    assert(false);
    return nullptr;
}

Value *emitExpression(IRBuilder<> &builder, ast::Expr &expr) {

    switch (expr.type()) {
    case ast::Expr::TYPE_INTEGER: return builder.getInt32(expr.getInteger());
    case ast::Expr::TYPE_INFIX:   return emitInfix(builder, expr.getInfix());
    case ast::Expr::TYPE_PREFIX:  return emitPrefix(builder, expr.getPrefix());
    case ast::Expr::TYPE_FLOATING: 
        {
            Constant *floating = ConstantFP::get(builder.getFloatTy(), expr.getFloating());
            return builder.CreateFPCast(floating, builder.getFloatTy());
            break;
        }
    case ast::Expr::TYPE_CALL:
        {
            auto call = expr.getCall();
            if (call.name == "pi") {
                Constant *piConst = ConstantFP::get(builder.getFloatTy(), M_PI);
                return builder.CreateFPCast(piConst, builder.getFloatTy());
            }

            break;
        }
    default: assert(false); break;
    }
                                            

    assert(false);
    return nullptr;
}
