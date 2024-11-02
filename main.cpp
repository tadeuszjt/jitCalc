// author: Tadeusz Tomoszek
// email : tadeuszjt@protonmail.com
// Implements a JIT compiled calculator.

#include <iostream>
#include <cassert>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/Support/TargetSelect.h>

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

    orc::ThreadSafeContext context(std::make_unique<LLVMContext>());

    // in ORCJit, you have to create a dynamic library to add/remove modules
    auto jit = cantFail(orc::LLJITBuilder().create());
    auto &dyLib = cantFail(jit->createJITDylib("jitCalc_dyLib"));


    SymbolTable symTab;

    for (;;) {
        std::string input = getNextInput();
        if (input == "q") {
            break;
        }

        auto result = parse(input);
        auto *presult = result.get();

        if (auto *expr = dynamic_cast<ast::Expr*>(presult)) {
            auto lock = context.getLock();

            Emit emit(*context.getContext(), "jitCalc_child");
            emit.getSymbolTable() = symTab;
            emit.startFunction("func");
            auto *v = emit.emitExpression(*expr);
            emit.emitPrint(v);
            emit.emitReturnNoBlock(emit.emitInt32(0));
            emit.mod().optimiseModule();
            emit.mod().printModule();

            auto tracker = dyLib.createResourceTracker();
            cantFail(jit->addIRModule(tracker, orc::ThreadSafeModule(emit.mod().moveModule(), context)));
            auto symbol = cantFail(jit->lookup(dyLib, "func"));
            auto funcPtr = symbol.toPtr<void(*)()>();
            funcPtr();
            cantFail(tracker->remove());

        } else if (auto *fnDef = dynamic_cast<ast::FnDef*>(presult)) {
            auto lock = context.getLock();

            Emit emit(*context.getContext(), "jitCalc_child");
            emit.setSymbolTable(symTab);
            emit.emitFuncDef(*fnDef);
            emit.mod().optimiseModule();
            emit.mod().printModule();
            symTab = emit.getSymbolTable();

            cantFail(jit->addIRModule(dyLib, orc::ThreadSafeModule(emit.mod().moveModule(), context)));
        }
    }


    return 0;
}
