#include "emit.h"

#include <cmath>




ModuleBuilder::ModuleBuilder(LLVMContext &context, const std::string &name)
    : irBuilder(context)
    , irModule(std::make_unique<Module>(name, context))
    , curFn(nullptr)
{
    // Declare the printf function
    Function::Create(
            FunctionType::get(irBuilder.getInt32Ty(), {irBuilder.getPtrTy()}, true),
            Function::ExternalLinkage,
            "printf",
            irModule.get());
}


void ModuleBuilder::createFunction(const std::string &name, Type *returnType) {
    FunctionType *fnType = FunctionType::get(returnType, {}, false);
    assert(fnType != nullptr);
    Function *fn = Function::Create(fnType, Function::ExternalLinkage, name, irModule.get());
    assert(fn != nullptr);
    BasicBlock *block = BasicBlock::Create(irModule->getContext(), "EntryBlock", fn);
    assert(block != nullptr);
}

void ModuleBuilder::setCurrentFunction(const std::string &name) {
    Function *fn = irModule->getFunction(name);
    assert(fn != nullptr);
    curFn = fn;
    irBuilder.SetInsertPoint(&fn->back());
}

void ModuleBuilder::emitReturn(Value *value) {
    assert(curFn != nullptr);
    irBuilder.CreateRet(value);
}

void ModuleBuilder::printModule() {
    irModule->print(outs(), nullptr);
}

void ModuleBuilder::emitPrint(Value *value) {
    std::vector<Value*> printfArgs;

    if (value->getType() == irBuilder.getInt32Ty()) {
        Value *format = irBuilder.CreateGlobalStringPtr("benis: %d\n");
        printfArgs = {format, value};
    }
    else if (value->getType() == irBuilder.getFloatTy()) {
        Value *format = irBuilder.CreateGlobalStringPtr("benis: %f\n");

        Value *doub = irBuilder.CreateFPExt(value, irBuilder.getDoubleTy());
        printfArgs = {format, doub};
    } else {
        assert(false);
    }

    irBuilder.CreateCall(irModule->getFunction("printf"), printfArgs);
}

Value* ModuleBuilder::emitExpression(const ast::Expr &expr) {
    if (expr.hasInteger()) {
        return irBuilder.getInt32(expr.getInteger());
    }
    if (expr.hasInfix()) {
        return emitInfix(expr.getInfix());
    }
    if (expr.hasPrefix()) {
        return emitPrefix(expr.getPrefix());
    }
    if (expr.hasFloating()) {
        Constant *floating = ConstantFP::get(irBuilder.getFloatTy(), expr.getFloating());
        return irBuilder.CreateFPCast(floating, irBuilder.getFloatTy());
    }
    if (expr.hasCall()) {
        auto call = expr.getCall();
        if (call.name == "pi") {
            Constant *piConst = ConstantFP::get(irBuilder.getFloatTy(), M_PI);
            return irBuilder.CreateFPCast(piConst, irBuilder.getFloatTy());
        }

        assert(false);
    }


    assert(false);
    return nullptr;
}

Value* ModuleBuilder::emitInfix(const ast::Infix &infix) {
    auto *left = emitExpression(*infix.left);
    auto *right = emitExpression(*infix.right);


    if (right->getType() == irBuilder.getInt32Ty() && left->getType() == irBuilder.getInt32Ty()) {
        switch (infix.symbol) { 
        case '+': return irBuilder.CreateAdd(left, right,  "infix");
        case '-': return irBuilder.CreateSub(left, right,  "infix");
        case '*': return irBuilder.CreateMul(left, right,  "infix");
        case '/': return irBuilder.CreateSDiv(left, right, "infix");
        default: assert(false); break;
        }
    }

    if (right->getType() == irBuilder.getFloatTy() && left->getType() == irBuilder.getFloatTy()) {
        switch (infix.symbol) { 
        case '+': return irBuilder.CreateFAdd(left, right,  "infix");
        case '-': return irBuilder.CreateFSub(left, right,  "infix");
        case '*': return irBuilder.CreateFMul(left, right,  "infix");
        case '/': return irBuilder.CreateFDiv(left, right, "infix");
        default: assert(false); break;
        }
    }


    if (
        (right->getType() == irBuilder.getInt32Ty() && left->getType() == irBuilder.getFloatTy()) ||
        (right->getType() == irBuilder.getFloatTy() && left->getType() == irBuilder.getInt32Ty())
    ) {
        Value *rightFloat = right;
        Value *leftFloat = left;

        if (right->getType() == irBuilder.getInt32Ty()) {
            rightFloat = irBuilder.CreateSIToFP(right, irBuilder.getFloatTy());
        }
        if (left->getType() == irBuilder.getInt32Ty()) {
            leftFloat = irBuilder.CreateSIToFP(left, irBuilder.getFloatTy());
        }

        switch (infix.symbol) { 
        case '+': return irBuilder.CreateFAdd(leftFloat, rightFloat,  "infix");
        case '-': return irBuilder.CreateFSub(leftFloat, rightFloat,  "infix");
        case '*': return irBuilder.CreateFMul(leftFloat, rightFloat,  "infix");
        case '/': return irBuilder.CreateFDiv(leftFloat, rightFloat, "infix");
        default: assert(false); break;
        }
    }



    assert(false);
    return nullptr;
}

Value* ModuleBuilder::emitPrefix(const ast::Prefix &prefix) {
    auto *right = emitExpression(*prefix.right);

    // Value is integer
    if (right->getType() == irBuilder.getInt32Ty()) {
        if (prefix.symbol == '-') {
            return irBuilder.CreateSub(irBuilder.getInt32(0), right, "prefix");
        }
        assert(false);
    }

    // Value is float
    if (right->getType() == irBuilder.getFloatTy()) {
        Constant *zeroConst = ConstantFP::get(irBuilder.getFloatTy(), 0.0);
        if (prefix.symbol == '-') {
            return irBuilder.CreateFSub(zeroConst, right, "prefix");
        }
        assert(false);
    }

    assert(false);
    return nullptr;
}
