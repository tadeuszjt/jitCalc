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
    builder.createCall("printf", args2);
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



void Emit::emitProgram(Sparse<ast::Node>::Key key, Sparse<ast::Node> &ast) {
    assert( std::holds_alternative<ast::Program>(ast.at(key)) );
    auto &prog = std::get<ast::Program>(ast.at(key));

    assert( std::holds_alternative<ast::List>(ast.at(prog.stmtList)) );
    auto &list = std::get<ast::List>(ast.at(prog.stmtList));

    for (auto key : list.list) {
        if (std::holds_alternative<ast::FnDef>(ast.at(key))) {
            emitFuncDef(ast, std::get<ast::FnDef>(ast.at(key)));
        } else {
            auto *value = emitExpression(ast, ast.at(key) );
            emitPrintf("result: %d\n", {value});
        }
    }

    emitReturnNoBlock(emitInt32(0));
}


void Emit::emitStmt(Sparse<ast::Node> &ast, const ast::Node &stmt) {
    if (std::holds_alternative<ast::FnDef>(stmt)) {
        emitFuncDef(ast, std::get<ast::FnDef>(stmt));

    } else if (std::holds_alternative<ast::Return>(stmt)) {
        auto *e = emitExpression(ast, ast.at(std::get<ast::Return>(stmt).expr));
        emitReturn(e);
    
    } else if (std::holds_alternative<ast::Let>(stmt)) {
        auto &let = std::get<ast::Let>(stmt);
        auto *expr = emitExpression(ast, ast.at(let.expr));
        auto key = builder.createVarLocalDebug(let.name.c_str());
        builder.setVarLocalDebugValue(let.pos, key, expr);

        define(let.name, ObjVar{.debugKey = key});
        writeVariable(symTab.look(let.name), builder.getCurrentBlock(), expr);

    } else if (std::holds_alternative<ast::Set>(stmt)) {
        auto &set = std::get<ast::Set>(stmt);
        auto *expr = emitExpression(ast, ast.at(set.expr));
        auto obj = look(set.name);
        assert(std::holds_alternative<ObjVar>(obj));

        builder.setVarLocalDebugValue(set.pos, std::get<ObjVar>(obj).debugKey, expr);
        writeVariable(symTab.look(set.name), builder.getCurrentBlock(), expr);

    } else if (std::holds_alternative<ast::If>(stmt)) {
        auto &if_ = std::get<ast::If>(stmt);

        auto *cnd = emitExpression(ast, ast.at(if_.cnd));
        auto *cmp = builder.ir().CreateICmpEQ(cnd, emitInt32(0));

        BasicBlock *trueBlk = builder.appendNewBlock();
        BasicBlock *falseBlk = builder.appendNewBlock();
        BasicBlock *end = builder.appendNewBlock();

        builder.ir().CreateCondBr(cmp, falseBlk, trueBlk);

        sealBlock(falseBlk);
        sealBlock(trueBlk);

        builder.setCurrentBlock(trueBlk);
        symTab.pushScope();

        auto &bodyList = std::get<ast::List>(ast.at(if_.trueBody));
        for (auto stmtKey : bodyList.list) {
            emitStmt(ast, ast.at(stmtKey));
        }
        symTab.popScope();
        builder.ir().CreateBr(end);

        builder.setCurrentBlock(falseBlk);
        symTab.pushScope();

        auto &falseBodyList = std::get<ast::List>(ast.at(if_.falseBody));
        for (auto stmtKey : falseBodyList.list) {
            emitStmt(ast, ast.at(stmtKey));
        }
        symTab.popScope();
        builder.ir().CreateBr(end);

        sealBlock(end);

        builder.setCurrentBlock(end);

    } else if (std::holds_alternative<ast::For>(stmt)) {
        auto &for_ = std::get<ast::For>(stmt);

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

        auto *val = emitExpression(ast, ast.at(for_.cnd));
        auto *cnd = builder.ir().CreateICmpSLT(idx, val);

        builder.ir().CreateCondBr(cnd, bdyBlk, endBlk);
        sealBlock(bdyBlk);

        builder.setCurrentBlock(bdyBlk);
        symTab.pushScope();

        auto &bodyList = std::get<ast::List>(ast.at(for_.body));

        for (auto stmtKey : bodyList.list) {
            emitStmt(ast, ast.at(stmtKey));
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


void Emit::emitFuncDef(Sparse<ast::Node> &ast, const ast::FnDef& fnDef) {
    auto &argsList = std::get<ast::List>(ast.at(fnDef.args));

    define(fnDef.name, ObjFunc{argsList.size(), false});
    auto funcOld = funcCurrent;
    funcCurrent = fnDef.name;

    std::vector<Type*> argTypes(argsList.size(), builder.ir().getInt32Ty());
    auto *fn = builder.createFunc(fnDef.pos, fnDef.name.c_str(), argTypes, builder.ir().getInt32Ty());
    fn->setPersonalityFn(builder.getFunc("__gxx_personality_v0"));
    builder.setCurrentFunc(fnDef.name.c_str());
    BasicBlock *entry = builder.getCurrentBlock();
    sealBlock(entry);

    symTab.pushScope();

    for (int i = 0; i < argsList.size(); i++) {
        auto &arg = std::get<ast::Ident>(ast.at(argsList.list[i]));
        auto key = builder.createArgDebug(arg.ident.c_str(), i + 1);

        define(arg.ident, ObjVar{.debugKey = key});

        builder.setVarLocalDebugValue(arg.pos, key, builder.getCurrentFuncArg(i));
        writeVariable(symTab.look(arg.ident), entry, builder.getCurrentFuncArg(i));
    }

    auto &bodyList = std::get<ast::List>(ast.at(fnDef.body));
    for (auto stmtKey : bodyList.list) {
        emitStmt(ast, ast.at(stmtKey) );
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




Value* Emit::emitCall(Sparse<ast::Node> &ast, const ast::Call &call, bool resume) {
    auto objFunc = std::get<ObjFunc>(look(call.name));
    auto &argList = std::get<ast::List>(ast.at(call.args));

    assert(argList.size() == objFunc.numArgs);

    std::vector<Value*> vals;
    for (auto exprKey : argList.list) {
        vals.push_back(emitExpression(ast, ast.at(exprKey)));
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

        auto *val = builder.createInvoke(call.pos, normalBlk, unwindBlk, call.name.c_str(), vals);

        builder.setCurrentBlock(unwindBlk);
        auto *gv = builder.getGlobalVariable("_ZTIi");

        auto *structType = llvm::StructType::get(builder.ir().getPtrTy(), builder.ir().getInt32Ty());
        auto *lp  = builder.ir().CreateLandingPad(structType, 1);
        lp->addClause(ConstantExpr::getBitCast(gv, builder.ir().getPtrTy()));
        lp->setCleanup(true);


        auto *lpPtr = builder.ir().CreateExtractValue(lp, {0});
        auto *lpSel = builder.ir().CreateExtractValue(lp, {1});

        auto *tid = builder.createCall(call.pos, "llvm.eh.typeid.for.p0", {gv});

        auto *match = builder.ir().CreateICmpEQ(tid, lpSel);
        builder.ir().CreateCondBr(match, catchBlk, errBlk);

        builder.setCurrentBlock(catchBlk);
        auto *payloadPtr = builder.createCall(call.pos, "__cxa_begin_catch", {lpPtr});
        auto *payload = builder.ir().CreateLoad(builder.ir().getInt32Ty(), payloadPtr, "payload");
        this->emitPrintf("caught exception: %d\n", {payload});
        builder.createCall(call.pos, "__cxa_end_catch", {});
        emitReturnNoBlock(emitInt32(0));


        builder.setCurrentBlock(errBlk);
        auto *selLess = builder.ir().CreateICmpSLT(lpSel, builder.ir().getInt32(0));
        this->emitPrintf("errBlk\n", {});
        builder.ir().CreateCondBr(selLess, unexpBlk, cleanBlk);


        builder.setCurrentBlock(unexpBlk);
        this->emitPrintf("unexpBlk\n", {});
        builder.createCall(call.pos, "__cxa_call_unexpected", {lpPtr});
        builder.ir().CreateUnreachable();


        builder.setCurrentBlock(cleanBlk);
        emitPrintf("cleanBlk\n", {});
        builder.ir().CreateResume(lp);

        builder.setCurrentBlock(normalBlk);
        return val;
    } else {
        return builder.createCall(call.pos, call.name.c_str(), vals);
    }
}

Value* Emit::emitExpression(Sparse<ast::Node> &ast, const ast::Node &expr) {
    if (std::holds_alternative<ast::Integer>(expr)) {
        return emitInt32( std::get<ast::Integer>(expr).integer);
    }
    if (std::holds_alternative<ast::Prefix>(expr)) {
        return emitPrefix(ast, std::get<ast::Prefix>(expr) );
    }
    if (std::holds_alternative<ast::Infix>(expr)) {
        return emitInfix(ast, std::get<ast::Infix>(expr));
    }
    if (std::holds_alternative<ast::Call>(expr)) {
        auto &call = std::get<ast::Call>(expr);
        return emitCall(ast, call, true);
    }
    if (std::holds_alternative<ast::Ident>(expr)) {
        auto &ident = std::get<ast::Ident>(expr);
        auto object = look(ident.ident);
        if (std::holds_alternative<ObjVar>(object)) {
            return readVariable(symTab.look(ident.ident), builder.getCurrentBlock());
        }

        assert(false);
    }
    assert(false);
    return nullptr;
}


Value* Emit::emitPrefix(Sparse<ast::Node> &ast, const ast::Prefix &prefix) {
    auto *right = emitExpression(ast, ast.at(prefix.right) );

    if (right->getType() == builder.ir().getInt32Ty()) {
        if (prefix.op == ast::Minus) {
            return builder.ir().CreateSub(emitInt32(0), right, "prefix");
        }
        assert(false);
    }
    
    assert(false);
    return nullptr;
}

Value* Emit::emitInfix(Sparse<ast::Node> & ast, const ast::Infix &infix) {
    auto *left = emitExpression(ast, ast.at(infix.left));
    auto *right = emitExpression(ast, ast.at(infix.right));

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
            infix.pos,
            "__cxa_allocate_exception",
            {builder.ir().getInt64(4)}
        );
        builder.ir().CreateStore(builder.ir().getInt32(123), eh);
        builder.createCall(
            infix.pos,
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


Value* Emit::emitInt32(int n) {
    return builder.ir().getInt32(n);
}

void Emit::startFunction(const std::string &name) {
    assert(startFunctionName == "");
    auto *fn = builder.createFunc(TextPos(0, 0, 0), name.c_str(), {}, builder.ir().getInt32Ty());
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
