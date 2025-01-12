#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Debug.h"
#include "PPProfiler.h"

using namespace llvm;

#define DEBUG_TYPE "ppprofiler"

ALWAYS_ENABLED_STATISTIC(NumOfFunc, "Number of instrumented functions.");

namespace {
class PPProfilerIRPass : public llvm::PassInfoMixin<PPProfilerIRPass> {
public:
    llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);

private:
    void instrument(llvm::Function &F, llvm::Function *EnterFn, llvm::Function *ExitFn);
};


void PPProfilerIRPass::instrument(Function &F, Function *EnterFn, Function *ExitFn) {
    NumOfFunc++;

    IRBuilder<> Builder(&*F.getEntryBlock().begin());

    GlobalVariable *FnName = Builder.CreateGlobalString(F.getName());

    Builder.CreateCall(EnterFn->getFunctionType(), EnterFn, {FnName});

    for (BasicBlock &BB : F) {
        for (Instruction &Inst : BB) {
            if (Inst.getOpcode() == Instruction::Ret) {
                Builder.SetInsertPoint(&Inst);
                Builder.CreateCall(ExitFn->getFunctionType(), ExitFn, {FnName});
            }
        }
    }
}

PreservedAnalyses PPProfilerIRPass::run(llvm::Module &M, ModuleAnalysisManager &AM) {
    if (M.getFunction("__ppp_enter") ||
        M.getFunction("__ppp_exit")) {
        return PreservedAnalyses::all();
    }

    Type *VoidTy = Type::getVoidTy(M.getContext());
    PointerType *PtrTy = PointerType::getUnqual(M.getContext());
    FunctionType *EnterExitFty = FunctionType::get(VoidTy, {PtrTy}, false);
    Function *EnterFn = Function::Create(
        EnterExitFty,
        GlobalValue::ExternalLinkage,
        "__ppp_enter",
        M
    );
    Function *ExitFn = Function::Create(
        EnterExitFty,
        GlobalVariable::ExternalLinkage,
        "__ppp_exit",
        M
    );

    for (auto &F : M.functions()) {
        if (!F.isDeclaration() && F.hasName()) {
            instrument(F, EnterFn, ExitFn);
        }
    }

    return PreservedAnalyses::none();
}
    
}


void RegisterCB(PassBuilder &PB) {
    PB.registerPipelineParsingCallback(
        [](StringRef Name, ModulePassManager &MPM, ArrayRef<PassBuilder::PipelineElement>) {
            if (Name == "ppprofiler") {
                MPM.addPass(PPProfilerIRPass());
                return true;
            }
            return false;
    });
}


llvm::PassPluginLibraryInfo getPPProfilerPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "PPProfiler", "v0.1", RegisterCB};
}


#ifndef LLVM_PROFILER_LINK_INTO_TOOLS
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return getPPProfilerPluginInfo();
}
#endif

