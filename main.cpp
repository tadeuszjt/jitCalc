// author: Tadeusz Tomoszek
// email : tadeuszjt@protonmail.com
// WIP. Will eventually implement a JIT compiled language for calculating numbers.

#include <iostream>
#include <cassert>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/MCJIT.h"

#include "llvm/Support/TargetSelect.h"

#include "lexer.h"
#include "ast.h"
#include "parse.h"
#include "grammar.tab.hh"

using namespace llvm;

Function* createMainFunction(Module *M, LLVMContext &context) {
    FunctionType *MainFnType = FunctionType::get(
        Type::getInt32Ty(context),
        {},     // arg list
        false); // varg

    Function *MainFn = Function::Create(MainFnType, Function::ExternalLinkage, "main", M);
    return MainFn;
}

static void emitReturn(LLVMContext &context, BasicBlock *BB, Value *value) { 
    ReturnInst::Create(context, value, BB);
}

static void emitPrint(LLVMContext &context, BasicBlock *BB, Value *value) {
}


static Value *emitExpression(IRBuilder<> &builder, ast::EExpression &expr) {

    switch (expr.type()) {
    case ast::EExpression::TYPE_INTEGER: return builder.getInt32(expr.getInteger());
    case ast::EExpression::TYPE_INFIX:
        {
            auto infix = expr.getInfix();
            auto *left = emitExpression(builder, *infix.left);
            auto *right = emitExpression(builder, *infix.right);

            switch (infix.symbol) { 
            case '+': return builder.CreateAdd(left, right,  "infix");
            case '-': return builder.CreateSub(left, right,  "infix");
            case '*': return builder.CreateMul(left, right,  "infix");
            case '/': return builder.CreateSDiv(left, right, "infix");
            default: assert(false); break;
            }

            break;
        }
    case ast::EExpression::TYPE_PREFIX:
        {
            auto prefix = expr.getPrefix();
            auto *right = emitExpression(builder, *prefix.right);
            auto *zero  = builder.getInt32(0);

            switch (prefix.symbol) { 
            case '-': return builder.CreateSub(zero, right,  "prefix");
            default: assert(false); break;
            }

            break;
        }
    default: assert(false); break;
    }
                                            

    assert(false);
    return nullptr;
}


int main(int argc, char **argv) {
    InitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    std::string input;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);

        if (input == "q") {
            break;
        }

        auto expr = parse(input);

        LLVMContext context;
        std::unique_ptr<Module> owner(new Module("jitCalc", context));
        Module *mod = owner.get();
        IRBuilder<> builder(context);

        auto *mainFn = createMainFunction(mod, context);
        BasicBlock *basicBlock = BasicBlock::Create(context, "EntryBlock", mainFn);
        builder.SetInsertPoint(basicBlock);

        auto *value = emitExpression(builder, expr);
        builder.CreateRet(value);

        ExecutionEngine *executionEngine = EngineBuilder(std::move(owner)).create();
        std::vector<GenericValue> noArgs;
        GenericValue result = executionEngine->runFunction(mainFn, noArgs);

        outs() << "Result: " << result.IntVal << "\n";
    }


    return 0;
}
