// author: Tadeusz Tomoszek
// email : tadeuszjt@protonmail.com
// WIP. Will eventually implement a JIT compiled language for calculating numbers.

#include <iostream>
#include <cassert>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>

#include "lexer.h"
#include "ast.h"
#include "parse.h"
#include "grammar.tab.hh"

using namespace llvm;

static Function* createMainFunction(Module *M, LLVMContext &context) {
    FunctionType *MainFnType = FunctionType::get(
        Type::getInt32Ty(context),
        {},     // arg list
        false); // varg

    Function *MainFn = Function::Create(MainFnType, Function::ExternalLinkage, "main", M);
    BasicBlock *BB = BasicBlock::Create(context, "EntryBlock", MainFn);
    Value *zero = ConstantInt::get(Type::getInt32Ty(context), 0);
    ReturnInst::Create(context, zero, BB);
    return MainFn;
}


int main(int argc, char **argv) {
    std::cout << "jitCalc" << std::endl;
    std::string text1 = "1234 + 34 * 34 + 2";
    std::string text2 = "342 * 23 / (4 + 6)";
    std::cout << parse(text1) << std::endl;
    std::cout << parse(text2) << std::endl;


    LLVMContext context;
    Module M("jitCalc", context);

    auto *fn = createMainFunction(&M, context);
    M.print(llvm::outs(), nullptr);
    return 0;
}
