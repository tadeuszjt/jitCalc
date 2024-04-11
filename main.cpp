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
#include "moduleBuilder.h"
#include "grammar.tab.hh"

using namespace llvm;

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
        input += "\n";
    }
}

int main(int argc, char **argv) {
    InitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMContext context;
    auto topModule = std::make_unique<Module>("jitCalc", context);
    ExecutionEngine *executionEngine = EngineBuilder(std::move(topModule)).create();

    for (;;) {
        std::string input = getNextInput();
        if (input == "q") {
            break;
        }

        auto result = parse(input);
        auto *presult = result.get();

        if (auto *expr = dynamic_cast<ast::Expr*>(presult)) {
            ModuleBuilder modBuilder(context, "jitCalc_child");

            modBuilder.createFunction("func", modBuilder.getInt32Ty());
            modBuilder.setCurrentFunction("func");

            auto *v = modBuilder.emitExpression(*expr);
            modBuilder.emitPrint(v);
            modBuilder.emitReturn(modBuilder.getInt32(0));
            modBuilder.printModule();

            // run module in JIT engine
            Module *mod = modBuilder.getModule();
            Function *fn = modBuilder.getFunction("func");
            executionEngine->addModule(modBuilder.moveModule());
            std::vector<GenericValue> noArgs;
            executionEngine->runFunction(fn, noArgs);
            executionEngine->removeModule(mod);
        } else if (auto *fnDef = dynamic_cast<ast::FnDef*>(presult)) {

            ModuleBuilder modBuilder(context, "jitCalc_" + fnDef->name->ident);
            modBuilder.createFunction(fnDef->name->ident, modBuilder.getFloatTy());
            modBuilder.setCurrentFunction(fnDef->name->ident);
            auto *v = modBuilder.emitExpression(*fnDef->body);
            auto *v2 = modBuilder.emitConvertToFloat(v);
            modBuilder.emitReturn(v2);

            executionEngine->addModule(modBuilder.moveModule());
        }
    }


    return 0;
}
