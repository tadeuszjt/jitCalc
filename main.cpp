// author: Tadeusz Tomoszek
// email : tadeuszjt@protonmail.com
// WIP. Will eventually implement a JIT compiled language for calculating numbers.

#include <iostream>
#include <cassert>
#include <cmath>
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
#include "emit.h"
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

        // Declare the printf function
        FunctionType* printfType = FunctionType::get(builder.getInt32Ty(), {builder.getPtrTy()}, true);
        Function* printfFunc = Function::Create(printfType, Function::ExternalLinkage, "printf", mod);


        auto *mainFn = createMainFunction(mod, context);
        BasicBlock *basicBlock = BasicBlock::Create(context, "EntryBlock", mainFn);
        builder.SetInsertPoint(basicBlock);
        auto *value = emitExpression(builder, expr);
        emitPrint(mod, builder, value);
        builder.CreateRet(builder.getInt32(0));


        ExecutionEngine *executionEngine = EngineBuilder(std::move(owner)).create();
        std::vector<GenericValue> noArgs;
        executionEngine->runFunction(mainFn, noArgs);
        //mod->print(outs(), nullptr);
    }


    return 0;
}
