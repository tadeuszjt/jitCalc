// author: Tadeusz Tomoszek
// email : tadeuszjt@protonmail.com
// Implements a JIT compiled calculator.

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


std::string getNextInput() {
    std::string input;

    for (;;) {
        std::cout << "> ";
        std::string line;
        std::getline(std::cin, line);

        if (line == "q") {
            return "q";
        }
        if (line == ";") {
            return input;
        }

        input += line;
    }
}


int main(int argc, char **argv) {
    InitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMContext context;
    auto topModule = std::make_unique<Module>("jitCalc", context);
    IRBuilder<> builder(context);
    ExecutionEngine *executionEngine = EngineBuilder(std::move(topModule)).create();


    for (;;) {
        std::string input = getNextInput();
        if (input == "q") {
            break;
        }

        auto expr = parse(input);

        auto mod = std::make_unique<Module>("jitCalc_child", context);
        auto *modPtr = mod.get();


        // Declare the printf function
        FunctionType* printfType = FunctionType::get(builder.getInt32Ty(), {builder.getPtrTy()}, true);
        Function* printfFunc = Function::Create(printfType, Function::ExternalLinkage, "printf", mod.get());


        auto *mainFn = createMainFunction(mod.get(), context);
        BasicBlock *basicBlock = BasicBlock::Create(context, "EntryBlock", mainFn);
        builder.SetInsertPoint(basicBlock);
        auto *value = emitExpression(builder, expr);
        emitPrint(mod.get(), builder, value);
        builder.CreateRet(builder.getInt32(0));

        // run module in JIT engine
        executionEngine->addModule(std::move(mod));
        std::vector<GenericValue> noArgs;
        executionEngine->runFunction(mainFn, noArgs);
        modPtr->print(outs(), nullptr);
        executionEngine->removeModule(modPtr);

    }


    return 0;
}
