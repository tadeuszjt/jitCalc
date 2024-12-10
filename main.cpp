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
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>

#include "lexer.h"
#include "ast.h"
#include "parse.h"
#include "emit.h"
#include "symbols.h"

using namespace llvm;

cl::opt<bool>        replMode("i", cl::desc("Interactive REPL Mode"));
cl::opt<bool>        emitLlFile("l", cl::desc("Emit LLVM IR file"));
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
        if (emitLlFile.getNumOccurrences() > 0) {
            llvm::errs() << "Cannot have output IR file in REPL mode (-l)\n";
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
        auto filePath = llvm::SmallString<128>(inputFile);
        assert(!llvm::sys::fs::make_absolute(filePath));

        auto buffer = std::move(*llvm::MemoryBuffer::getFile(filePath));
        assert(buffer);


        auto *prog = parse(*buffer);
        auto lock = context.getLock();
        Emit emit(*context.getContext(), "jitCalc_child", filePath.c_str());

        emit.startFunction("main");
        emit.emitProgram(*prog);
        emit.mod().finaliseDebug();

        emit.mod().printModule();
        puts("");
        emit.mod().verifyModule();
        //emit.mod().optimiseModule();

        if (emitLlFile) {
            auto llFilePath = filePath;
            llFilePath.append(".ll");

            std::error_code errorCode;
            llvm::raw_fd_ostream llFile(llFilePath.c_str(), errorCode, llvm::sys::fs::OF_Text);
            assert(!errorCode);
            emit.mod().getLlModule().print(llFile, nullptr);


        } else {
            auto tracker = dyLib.createResourceTracker();
            cantFail(jit->addIRModule(tracker, orc::ThreadSafeModule(emit.mod().moveModule(), context)));
            auto symbol = cantFail(jit->lookup(dyLib, "main"));
            auto funcPtr = symbol.toPtr<void(*)()>();
            funcPtr();
            cantFail(tracker->remove());
        }
    } else {
        for (int i = 0;; i++) {
            auto buffer = getNextInput();
            if (buffer->getBuffer() == "q") {
                break;
            }

            auto *prog = parse(*buffer);
            if (prog == nullptr) {
                continue;
            }

            auto lock = context.getLock();
            Emit emit(*context.getContext(), "jitCalc_child");
            emit.addFuncDefs(funcDefs);

            std::string funcName = "main" + std::to_string(i);
            emit.startFunction(funcName);
            emit.emitProgram(*prog);

            emit.mod().printModule();
            emit.mod().verifyModule();
            emit.mod().optimiseModule();

            funcDefs = emit.getFuncDefs();

            auto tracker = dyLib.createResourceTracker();
            cantFail(jit->addIRModule(tracker, orc::ThreadSafeModule(emit.mod().moveModule(), context)));
            auto symbol = cantFail(jit->lookup(dyLib, funcName.c_str()));
            auto funcPtr = symbol.toPtr<void(*)()>();
            funcPtr();
            //cantFail(tracker->remove());
        }
    }


    return 0;
}
