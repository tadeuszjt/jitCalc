#include "emit.h"

#include <cmath>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>

using namespace llvm;

void Emit::emitPrintf(const char* fmt, std::vector<llvm::Value*> args) {
    llvm::Value *ft = builder.ir().CreateGlobalString(fmt, "fmt");
    std::vector args2 = {ft};
    for (auto arg : args) {
        args2.push_back(arg);
    }
    builder.createCall(0, 0, "printf", args2);
}


Emit::Emit(LLVMContext &context, const std::string &name, const std::filesystem::path &debugFilePath)
    : builder(context, name, debugFilePath) {

    builder.createFuncDeclaration("printf", builder.ir().getInt32Ty(), {builder.ir().getPtrTy()}, true);
    builder.createFuncDeclaration(
        "__cxa_allocate_exception",
        builder.ir().getPtrTy(),
        {builder.ir().getInt64Ty()},
        false);
    builder.createFuncDeclaration(
        "__cxa_throw",
        builder.ir().getVoidTy(),
        {builder.ir().getPtrTy(), builder.ir().getPtrTy(), builder.ir().getPtrTy()},
        false);
    builder.createFuncDeclaration(
        "__cxa_begin_catch",
        builder.ir().getPtrTy(),
        {builder.ir().getPtrTy()},
        false);
    builder.createFuncDeclaration(
        "__cxa_call_unexpected",
        builder.ir().getVoidTy(),
        {builder.ir().getPtrTy()},
        false);
    builder.createFuncDeclaration("__cxa_end_catch", builder.ir().getVoidTy(), {}, false);
    builder.createFuncDeclaration("__gxx_personality_v0",  builder.ir().getInt32Ty(), {}, false);
    builder.createFuncDeclaration(
        "llvm.eh.typeid.for.p0",
        builder.ir().getInt32Ty(),
        {builder.ir().getPtrTy()},
        false);

    builder.createGlobalDeclaration("_ZTIi", builder.ir().getPtrTy());

}



void Emit::emitProgram(const ast::Program &program) {
    for (ast::Node *stmtPtr : (program.stmts)->list) {

        if (auto *fnDef = dyn_cast<ast::FnDef>(stmtPtr)) {
            emitFuncDef(*fnDef);
        } else {
            auto *v = emitExpression(*stmtPtr);
            this->emitPrintf("result: %d\n", {v});
        }
    }
    emitReturnNoBlock(emitInt32(0));
}


void Emit::emitStmt(const ast::Node &stmt) {
    if (auto *fnDef = dyn_cast<ast::FnDef>(&stmt)) {
        emitFuncDef(*fnDef);

    } else if (auto *ret = dyn_cast<ast::Return>(&stmt)) {
        printf("Returning at line: %d, col: %d\n", ret->getPos().line, ret->getPos().column);
        auto *e = emitExpression(*ret->expr);
        emitReturn(e);

    } else if (auto *let = dyn_cast<ast::Let>(&stmt)) {
        auto *expr = emitExpression(*let->expr);
        auto key = builder.createVarLocalDebug(let->name.c_str());
        builder.setVarLocalDebugValue(key, expr);

        define(let->name, ObjVar{.debugKey = key});
        writeVariable(symTab.look(let->name), builder.getCurrentBlock(), expr);

    } else if (auto *set = dyn_cast<ast::Set>(&stmt)) {
        auto *expr = emitExpression(*set->expr);
        auto obj = look(set->name);
        assert(std::holds_alternative<ObjVar>(obj));

        builder.setVarLocalDebugValue(std::get<ObjVar>(obj).debugKey, expr);
        writeVariable(symTab.look(set->name), builder.getCurrentBlock(), expr);

    } else if (auto *if_ = dyn_cast<ast::If>(&stmt)) {
        auto *cnd = emitExpression(*if_->cnd);
        auto *cmp = builder.ir().CreateICmpEQ(cnd, emitInt32(0));

        BasicBlock *trueBlk = builder.appendNewBlock();
        BasicBlock *falseBlk = builder.appendNewBlock();
        BasicBlock *end = builder.appendNewBlock();

        builder.ir().CreateCondBr(cmp, falseBlk, trueBlk);

        sealBlock(falseBlk);
        sealBlock(trueBlk);

        builder.setCurrentBlock(trueBlk);
        symTab.pushScope();

        for (ast::Node *stmtPtr : (*if_->trueBody).list) {
            emitStmt(*stmtPtr);
        }
        symTab.popScope();
        builder.ir().CreateBr(end);

        builder.setCurrentBlock(falseBlk);
        symTab.pushScope();

        for (ast::Node *stmtPtr : (*if_->falseBody).list) {
            emitStmt(*stmtPtr);
        }
        symTab.popScope();
        builder.ir().CreateBr(end);

        sealBlock(end);

        builder.setCurrentBlock(end);

    } else if (auto *for_ = dyn_cast<ast::For>(&stmt)) {
        BasicBlock *curBlk = builder.getCurrentBlock();
        BasicBlock *forBlk = builder.appendNewBlock("for");
        BasicBlock *bdyBlk = builder.appendNewBlock("body");
        BasicBlock *bdyEndBlk = builder.appendNewBlock("bodyEnd");
        BasicBlock *endBlk = builder.appendNewBlock("end");

        builder.ir().CreateBr(forBlk);
        builder.setCurrentBlock(forBlk);

        PHINode *idx = builder.ir().CreatePHI(builder.ir().getInt32Ty(), 2);
        idx->addIncoming(builder.ir().getInt32(0), curBlk);
        auto *idx2 = builder.ir().CreateAdd(idx, builder.ir().getInt32(1),  "increment");
        idx->addIncoming(idx2, bdyEndBlk);

        auto *val = emitExpression(*for_->cnd);
        auto *cnd = builder.ir().CreateICmpSLT(idx, val);

        builder.ir().CreateCondBr(cnd, bdyBlk, endBlk);
        sealBlock(bdyBlk);

        builder.setCurrentBlock(bdyBlk);
        symTab.pushScope();

        for (ast::Node *stmtPtr : (*for_->body).list) {
            emitStmt(*stmtPtr);
        }
        symTab.popScope();
        builder.ir().CreateBr(bdyEndBlk);
        sealBlock(bdyEndBlk);

        builder.setCurrentBlock(bdyEndBlk);
        builder.ir().CreateBr(forBlk);
        sealBlock(forBlk);

        builder.setCurrentBlock(endBlk);
        sealBlock(endBlk);

    } else {
        assert(false);
    }
}


void Emit::emitFuncDef(const ast::FnDef& fnDef) {
    define(fnDef.name->ident, ObjFunc{fnDef.args->size(), false});
    auto funcOld = funcCurrent;
    funcCurrent = fnDef.name->ident;

    std::vector<Type*> argTypes(fnDef.args->size(), builder.ir().getInt32Ty());
    auto *fn = builder.createFunc(fnDef.name->ident.c_str(), argTypes, builder.ir().getInt32Ty());
    fn->setPersonalityFn(builder.getFunc("__gxx_personality_v0"));
    builder.setCurrentFunc(fnDef.name->ident.c_str());
    BasicBlock *entry = builder.getCurrentBlock();
    sealBlock(entry);

    symTab.pushScope();

    for (int i = 0; i < fnDef.args->size(); i++) {
        auto key = builder.createArgDebug((*fnDef.args).list[i]->ident.c_str(), i + 1);

        define((*fnDef.args).list[i]->ident, ObjVar{.debugKey = key});

        builder.setVarLocalDebugValue(key, builder.getCurrentFuncArg(i));
        writeVariable(symTab.look((*fnDef.args).list[i]->ident), entry, builder.getCurrentFuncArg(i));
    }

    for (ast::Node *stmtPtr : (*fnDef.body).list) {
        emitStmt(*stmtPtr);
    }

    symTab.popScope();
    emitReturnNoBlock(emitInt32(0));
    funcCurrent = funcOld;
    if (funcCurrent == "") {
        builder.setCurrentFunc(startFunctionName);
    } else {
        builder.setCurrentFunc(funcCurrent.c_str());
    }
}


Value* Emit::emitInt32(int n) {
    return builder.ir().getInt32(n);
}

void Emit::startFunction(const std::string &name) {
    assert(startFunctionName == "");
    auto *fn = builder.createFunc(name.c_str(), {}, builder.ir().getInt32Ty());
    fn->setPersonalityFn(builder.getFunc("__gxx_personality_v0"));
    builder.setCurrentFunc(name.c_str());
    startFunctionName = name;
}

void Emit::emitReturn(Value *value) {
    BasicBlock *emptyBlock = builder.appendNewBlock();
    builder.ir().CreateRet(value);
    builder.setCurrentBlock(emptyBlock);
}

void Emit::emitReturnNoBlock(Value *value) {
    builder.ir().CreateRet(value);
}



Value* Emit::emitCall(const ast::Call &call, bool resume) {
    auto object = look(call.name);
    assert(std::holds_alternative<ObjFunc>(object));

    auto objFunc = std::get<ObjFunc>(object);

    assert(call.args->size() == objFunc.numArgs);

    std::vector<Value*> vals;
    for (auto exprPtr : (*call.args).list) {
        vals.push_back(emitExpression(*exprPtr));
    }
 
    std::vector<Type*> argTypes(objFunc.numArgs, builder.ir().getInt32Ty());

    if (builder.getFunc(call.name.c_str()) == nullptr) {
        builder.createFuncDeclaration(call.name.c_str(), builder.ir().getInt32Ty(), argTypes, false);
    }

    if (objFunc.hasException) {
        BasicBlock* normalBlk = builder.appendNewBlock("normal");
        BasicBlock* unwindBlk = builder.appendNewBlock("unwind");
        BasicBlock* catchBlk  = builder.appendNewBlock("catch");
        BasicBlock* errBlk    = builder.appendNewBlock("error");
        BasicBlock* unexpBlk  = builder.appendNewBlock("unexpected");
        BasicBlock* cleanBlk  = builder.appendNewBlock("cleanup");

        sealBlock(normalBlk);
        sealBlock(unwindBlk);
        sealBlock(catchBlk);
        sealBlock(errBlk);
        sealBlock(unexpBlk);
        sealBlock(cleanBlk);

        auto *val = builder.createInvoke(
            call.getPos().line, call.getPos().column, normalBlk, unwindBlk, call.name.c_str(), vals);

        builder.setCurrentBlock(unwindBlk);
        auto *gv = builder.getGlobalVariable("_ZTIi");

        auto *structType = llvm::StructType::get(builder.ir().getPtrTy(), builder.ir().getInt32Ty());
        auto *lp  = builder.ir().CreateLandingPad(structType, 1);
        lp->addClause(ConstantExpr::getBitCast(gv, builder.ir().getPtrTy()));
        lp->setCleanup(true);


        auto *lpPtr = builder.ir().CreateExtractValue(lp, {0});
        auto *lpSel = builder.ir().CreateExtractValue(lp, {1});

        auto *tid = builder.createCall(
            call.getPos().line, call.getPos().column, "llvm.eh.typeid.for.p0", {gv});

        auto *match = builder.ir().CreateICmpEQ(tid, lpSel);
        builder.ir().CreateCondBr(match, catchBlk, errBlk);

        builder.setCurrentBlock(catchBlk);
        auto *payloadPtr = builder.createCall(
            call.getPos().line, call.getPos().column, "__cxa_begin_catch", {lpPtr});
        auto *payload = builder.ir().CreateLoad(builder.ir().getInt32Ty(), payloadPtr, "payload");
        this->emitPrintf("caught exception: %d\n", {payload});
        builder.createCall(
            call.getPos().line, call.getPos().column, "__cxa_end_catch", {});
        emitReturnNoBlock(emitInt32(0));


        builder.setCurrentBlock(errBlk);
        auto *selLess = builder.ir().CreateICmpSLT(lpSel, builder.ir().getInt32(0));
        this->emitPrintf("errBlk\n", {});
        builder.ir().CreateCondBr(selLess, unexpBlk, cleanBlk);


        builder.setCurrentBlock(unexpBlk);
        this->emitPrintf("unexpBlk\n", {});
        builder.createCall(
            call.getPos().line, call.getPos().column, "__cxa_call_unexpected", {lpPtr});
        builder.ir().CreateUnreachable();


        builder.setCurrentBlock(cleanBlk);
        this->emitPrintf("cleanBlk\n", {});
        builder.ir().CreateResume(lp);

        builder.setCurrentBlock(normalBlk);
        return val;
    } else {
        return builder.createCall(call.getPos().line, call.getPos().column, call.name.c_str(), vals);
    }
}

Value* Emit::emitExpression(const ast::Node &expr) {
    if (auto *integer = dyn_cast<ast::Integer>(&expr)) {
        return emitInt32(integer->integer);
    }
    if (auto *floating = dyn_cast<ast::Floating>(&expr)) {
        assert(false);
    }
    if (auto *infix = dyn_cast<ast::Infix>(&expr)) {
        return emitInfix(*infix);
    }
    if (auto *prefix = dyn_cast<ast::Prefix>(&expr)) {
        return emitPrefix(*prefix);
    }
    if (auto *call = dyn_cast<ast::Call>(&expr)) {
        return emitCall(*call, true);
    }
    if (auto *ident = dyn_cast<ast::Ident>(&expr)) {
        auto object = look(ident->ident);
        if (std::holds_alternative<ObjVar>(object)) {
            return readVariable(symTab.look(ident->ident), builder.getCurrentBlock());
        }

        assert(false);
    }
    assert(false);
    return nullptr;
}

Value* Emit::emitInfix(const ast::Infix &infix) {
    auto *left = emitExpression(*infix.left);
    auto *right = emitExpression(*infix.right);

    switch (infix.op) { 
    case ast::Plus: return builder.ir().CreateAdd(left, right,  "infix");
    case ast::Minus: return builder.ir().CreateSub(left, right,  "infix");
    case ast::Times: return builder.ir().CreateMul(left, right,  "infix");
    case ast::Divide: {
        // function has exception
        if (funcCurrent != "") {
            auto object = look(funcCurrent);
            assert(std::holds_alternative<ObjFunc>(object));
            auto objFn = std::get<ObjFunc>(object);
            redefine(funcCurrent, ObjFunc{objFn.numArgs, true});
        }


        // throw exception on 0
        auto *cmp = builder.ir().CreateICmpEQ(right, builder.ir().getInt32(0));

        BasicBlock *zeroBlk = builder.appendNewBlock("div_zero");
        BasicBlock *okayBlk = builder.appendNewBlock("div_okay");

        sealBlock(zeroBlk);
        sealBlock(okayBlk);

        builder.ir().CreateCondBr(cmp, zeroBlk, okayBlk);

        builder.setCurrentBlock(zeroBlk);

        // throw exception
        auto *eh = builder.createCall(
            infix.getPos().line,
            infix.getPos().column,
            "__cxa_allocate_exception",
            {builder.ir().getInt64(4)}
        );
        builder.ir().CreateStore(builder.ir().getInt32(123), eh);
        builder.createCall(
            infix.getPos().line, infix.getPos().column,
            "__cxa_throw",
            {eh, builder.getGlobalVariable("_ZTIi"),
            builder.getNullptr()}
        );
        builder.ir().CreateUnreachable();

        builder.setCurrentBlock(okayBlk);

        return builder.ir().CreateSDiv(left, right, "infix");
    }
    case ast::LT: {
        auto *cmp = builder.ir().CreateICmpSLT(left, right);
        auto *i32 = builder.ir().CreateZExt(cmp, builder.ir().getInt32Ty());
        return i32;
    }
    case ast::GT: {
        auto *cmp = builder.ir().CreateICmpSGT(left, right);
        auto *i32 = builder.ir().CreateZExt(cmp, builder.ir().getInt32Ty());
        return i32;
    }
    case ast::EqEq: {
        auto *cmp = builder.ir().CreateICmpEQ(left, right);
        auto *i32 = builder.ir().CreateZExt(cmp, builder.ir().getInt32Ty());
        return i32;
    }
    default: assert(false); break;
    }

    assert(false);
    return nullptr;
}

Value* Emit::emitPrefix(const ast::Prefix &prefix) {
    auto *right = emitExpression(*prefix.right);

    if (right->getType() == builder.ir().getInt32Ty()) {
        if (prefix.op == ast::Minus) {
            return builder.ir().CreateSub(emitInt32(0), right, "prefix");
        }
        assert(false);
    }
    
    assert(false);
    return nullptr;
}

void Emit::define(const std::string &name, Object object) {
    assert(not symTab.isDefined(name));

    auto id = symTab.insert(name);
    assert(objTable.find(id) == objTable.end());
    objTable[id] = object;
}

void Emit::redefine(const std::string &name, Object object) {
    assert(symTab.isDefined(name));
    auto id = symTab.look(name);
    assert(objTable.find(id) != objTable.end());
    objTable[id] = object;
}

Object Emit::look(const std::string &name) {
    auto id = symTab.look(name);
    if (objTable.find(id) == objTable.end()) {
        assert(false);
    }
    return objTable[id];
}

void Emit::sealBlock(BasicBlock *block) {
    assert(sealedBlocks.find(block) == sealedBlocks.end());

    if (incompletePhis.find(block) != incompletePhis.end()) {
        for (auto &pair : incompletePhis[block]) {
            addPhiOperands(pair.first, incompletePhis[block][pair.first]);
        }
    }
    sealedBlocks.insert(block);
}

void Emit::writeVariable(SymbolTable::ID variable, BasicBlock* block, Value *value) {
    currentDefs[variable][block] = value;
}

Value* Emit::readVariable(SymbolTable::ID variable, BasicBlock* block) {
    if (currentDefs.find(variable) != currentDefs.end()) { // local value numbering
        if (currentDefs[variable].find(block) != currentDefs[variable].end()) {
            return currentDefs[variable][block];
        }
    }

    return readVariableRecursive(variable, block);
}


Value* Emit::readVariableRecursive(SymbolTable::ID variable, BasicBlock* block) {
    Value *val;

    if (sealedBlocks.find(block) == sealedBlocks.end()) {
        auto *phi = newPhi(block);
        incompletePhis[block][variable] = phi;
        val = phi;
    } else if (pred_size(block) == 1) {
        val = readVariable(variable, *pred_begin(block));
    } else {
        val = newPhi(block);
        writeVariable(variable, block, val);
        addPhiOperands(variable, (PHINode*)val);
    }

    writeVariable(variable, block, val);
    return val;
}

PHINode *Emit::newPhi(BasicBlock* block) {
    auto prevInsertionPoint = builder.ir().saveIP();
    if (!block->empty()) {
        builder.ir().SetInsertPoint(&block->front());
    } else {
        builder.ir().SetInsertPoint(block);
    }
    auto *phi = builder.ir().CreatePHI(builder.ir().getInt32Ty(), 0, "phi");
    assert(phi != nullptr);
    builder.ir().restoreIP(prevInsertionPoint);
    return phi;
}

void Emit::addPhiOperands(SymbolTable::ID variable, PHINode *phi) {
    for (auto *pred : predecessors(phi->getParent())) {
        phi->addIncoming(readVariable(variable, pred), pred);
    }
}
