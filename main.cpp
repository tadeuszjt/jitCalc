// author: Tadeusz Tomoszek
// email : tadeuszjt@protonmail.com
// Implements a JIT compiled calculator.

#include <iostream>
#include <cassert>
#include <filesystem>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/CommandLine.h>

#include "lexer.h"
#include "ast.h"
#include "parse.h"
#include "emit.h"
#include "symbols.h"

using namespace llvm;

cl::opt<bool>        replMode("i", cl::desc("Interactive REPL Mode"));
cl::opt<std::string> inputFile(cl::Positional, cl::desc("<input file>"));

std::unique_ptr<llvm::MemoryBuffer> getNextInput() {
    std::string input;

    for (;;) {
        std::cout << "> ";
        std::string line;
        std::getline(std::cin, line);

        if (line == "q") {
            input = "q";
            break;
        }
        if (line == ";") {
            break;
        }

        input += line;
        input += "\n";
    }

    return llvm::MemoryBuffer::getMemBufferCopy(input);
}


int main(int argc, char **argv) {
    // parse command line arguments
    cl::ParseCommandLineOptions(argc, argv, "jitCalc\n");
    if (replMode) {
        if (inputFile.getNumOccurrences() > 0) {
            llvm::errs() << "Cannot have input file in REPL mode (-i)\n";
            return -1;
        }
    } else {
        if (inputFile.getNumOccurrences() != 1) {
            llvm::errs() << "Need one input file\n";
            return -1;
        }
    }


    InitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    orc::ThreadSafeContext context(std::make_unique<LLVMContext>());

    // in ORCJit, you have to create a dynamic library to add/remove modules
    auto jit = cantFail(orc::LLJITBuilder().create());
    auto &dyLib = cantFail(jit->createJITDylib("jitCalc_dyLib"));

    std::vector<std::pair<std::string, ObjFunc>> funcDefs;

    if (not replMode) {
        auto filepath = std::filesystem::absolute(std::string(inputFile));
        auto buffer = std::move(*llvm::MemoryBuffer::getFile(filepath.c_str()));
        assert(buffer);

        auto *result = parse(*buffer);
        assert(result);
        auto lock = context.getLock();
        Emit emit(*context.getContext(), "jitCalc_child");

        auto *prog = llvm::dyn_cast<ast::Program>(result);
        assert(prog);

        emit.startFunction("func");
        emit.emitProgram(*prog);
        emit.mod().verifyModule();
        emit.mod().optimiseModule();
        emit.mod().printModule();

        auto tracker = dyLib.createResourceTracker();
        cantFail(jit->addIRModule(tracker, orc::ThreadSafeModule(emit.mod().moveModule(), context)));
        auto symbol = cantFail(jit->lookup(dyLib, "func"));
        auto funcPtr = symbol.toPtr<void(*)()>();
        funcPtr();
        cantFail(tracker->remove());
    } else {
        for (;;) {
            auto buffer = getNextInput();
            if (buffer->getBuffer() == "q") {
                break;
            }

            auto *result = parse(*buffer);
            if (result == nullptr) {
                continue;
            }

            auto lock = context.getLock();
            Emit emit(*context.getContext(), "jitCalc_child");
            emit.addFuncDefs(funcDefs);

            if (auto *fnDef = llvm::dyn_cast<ast::FnDef>(result)) {
                emit.emitFuncDef(*fnDef);
            } else {
                emit.startFunction("func");
                auto *v = emit.emitExpression(*result);
                emit.printf("result: %d\n", {v});
                emit.emitReturnNoBlock(emit.emitInt32(0));
            }

            emit.mod().printModule();
            emit.mod().verifyModule();
            emit.mod().optimiseModule();

            funcDefs = emit.getFuncDefs();

            if (auto *fnDef = llvm::dyn_cast<ast::FnDef>(result)) {
                cantFail(jit->addIRModule(dyLib, orc::ThreadSafeModule(emit.mod().moveModule(), context)));
            } else {
                auto tracker = dyLib.createResourceTracker();
                cantFail(jit->addIRModule(tracker, orc::ThreadSafeModule(emit.mod().moveModule(), context)));
                auto symbol = cantFail(jit->lookup(dyLib, "func"));
                auto funcPtr = symbol.toPtr<void(*)()>();
                funcPtr();
                cantFail(tracker->remove());
            }
        }
    }


    return 0;
}
