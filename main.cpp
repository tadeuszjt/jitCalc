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
#include "symbols.h"

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

    SymbolTable symTab;

    for (;;) {
        std::string input = getNextInput();
        if (input == "q") {
            break;
        }

        auto result = parse(input);
        auto *presult = result.get();

        if (auto *expr = dynamic_cast<ast::Expr*>(presult)) {
            Emit emit(context, "jitCalc_child");
            emit.getSymbolTable() = symTab;
            emit.startFunction("func");
            auto *v = emit.emitExpression(*expr);
            emit.emitPrint(v);
            emit.emitReturnNoBlock(emit.emitInt32(0));

            // run module in JIT engine
            emit.mod().printModule();
            Module *mod = emit.mod().getModule();
            Function *fn = emit.mod().getFunction("func");
            executionEngine->addModule(emit.mod().moveModule());
            std::vector<GenericValue> noArgs;
            executionEngine->runFunction(fn, noArgs);
            executionEngine->removeModule(mod);

        } else if (auto *fnDef = dynamic_cast<ast::FnDef*>(presult)) {
            Emit emit(context, "jitCalc_child");
            emit.setSymbolTable(symTab);
            emit.emitFuncDef(*fnDef);
            emit.mod().printModule();
            symTab = emit.getSymbolTable();

            executionEngine->addModule(emit.mod().moveModule());
        }
    }


    return 0;
}
