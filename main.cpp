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

        auto program = parse(input);


        if (std::holds_alternative<ast::Expr>(program)) {
            ModuleBuilder modBuilder(context, "jitCalc_child");

            modBuilder.createFunction("func", modBuilder.getInt32Ty());
            modBuilder.setCurrentFunction("func");

            auto expr = std::get<ast::Expr>(program);
            auto *v = modBuilder.emitExpression(expr);
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

        } else if (std::holds_alternative<ast::Stmt>(program)) {
            auto stmt = std::get<ast::Stmt>(program);
            if (stmt.hasFnDef()) {
                auto fnDef = stmt.getFnDef();

                ModuleBuilder modBuilder(context, "jitCalc_" + fnDef.name);
                modBuilder.createFunction(fnDef.name, modBuilder.getFloatTy());
                modBuilder.setCurrentFunction(fnDef.name);
                auto *v = modBuilder.emitExpression(fnDef.body);
                auto *v2 = modBuilder.emitConvertToFloat(v);
                modBuilder.emitReturn(v2);

                executionEngine->addModule(modBuilder.moveModule());
            } else {
                assert(false);
            }



        } else {
            assert(false);
        }

    }


    return 0;
}
