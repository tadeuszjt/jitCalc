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
#include <llvm/Support/Casting.h>

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

    std::vector<std::pair<std::string, int>> funcDefs;

    for (;;) {
        std::string input = getNextInput();
        if (input == "q") {
            break;
        }

        auto *result = parse(input);
        if (result == nullptr) {
            continue;
        }

        if (auto *fnDef = llvm::dyn_cast<ast::FnDef>(result)) {
            auto lock = context.getLock();

            Emit emit(*context.getContext(), "jitCalc_child");
            emit.addFuncDefs(funcDefs);
            emit.emitFuncDef(*fnDef);
            emit.mod().printModule();
            emit.mod().verifyModule();
            emit.mod().optimiseModule();

            funcDefs = emit.getFuncDefs();

            cantFail(jit->addIRModule(dyLib, orc::ThreadSafeModule(emit.mod().moveModule(), context)));
        } else {
            auto lock = context.getLock();

            Emit emit(*context.getContext(), "jitCalc_child");
            emit.addFuncDefs(funcDefs);
            emit.startFunction("func");
            auto *v = emit.emitExpression(*result);
            emit.emitPrint(v);
            emit.emitReturnNoBlock(emit.emitInt32(0));
            emit.mod().printModule();
            emit.mod().verifyModule();
            emit.mod().optimiseModule();

            auto tracker = dyLib.createResourceTracker();
            cantFail(jit->addIRModule(tracker, orc::ThreadSafeModule(emit.mod().moveModule(), context)));
            auto symbol = cantFail(jit->lookup(dyLib, "func"));
            auto funcPtr = symbol.toPtr<void(*)()>();
            funcPtr();
            cantFail(tracker->remove());
        }

    }


    return 0;
}
