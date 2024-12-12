#pragma once

#include "moduleBuilder.h"
#include "ast.h"
#include "symbols.h"
#include "sparse.h"
#include "lexer.h"

#include <set>
#include <map>
#include <vector>

struct ObjFunc {
    size_t numArgs;
    bool   hasException;
};

struct ObjVar {
    Sparse<ModuleBuilder::VarLocal>::Key debugKey;
};

using Object = std::variant<ObjFunc, ObjVar>;

class Emit {
public:
    Emit(
        llvm::LLVMContext &context,
        const std::string &name,
        const std::filesystem::path &debugFilePath = "jitCalc"
    );

    void startFunction(const std::string &name);
    void emitPrintf(const char* fmt, std::vector<llvm::Value*> args);

    void         emitProgram(Sparse<ast::Node>::Key, Sparse<ast::Node> &);

    void         emitStmt(Sparse<ast::Node> &, const ast::Node &);
    void         emitFuncDef(Sparse<ast::Node> &, const ast::FnDef &);
    llvm::Value* emitExpression(Sparse<ast::Node> &, const ast::Node &);
    llvm::Value* emitInfix(Sparse<ast::Node> &, const ast::Infix &);
    llvm::Value* emitPrefix(Sparse<ast::Node> &, const ast::Prefix &);
    llvm::Value* emitInt32(int n);
    llvm::Value* emitCall(Sparse<ast::Node> &, const ast::Call&, bool);
    void         emitReturn(llvm::Value *value);
    void         emitReturnNoBlock(llvm::Value *value);

    std::vector<std::pair<std::string, ObjFunc>> getFuncDefs() {
        std::vector<std::pair<std::string, ObjFunc>> funcDefs;

        auto &map = symTab.getScope(0);
        for (auto &pair : map) {
            auto object = objTable[pair.second];
            if (std::holds_alternative<ObjFunc>(object)) {
                auto fn = std::get<ObjFunc>(object);
                funcDefs.push_back(std::make_pair<std::string, ObjFunc>(
                    std::string(pair.first),
                    ObjFunc{fn.numArgs, fn.hasException}
                ));
            }
        }
        return funcDefs;
    }


    void addFuncDefs(std::vector<std::pair<std::string, ObjFunc>> &funcs) {
        for (const auto &pair : funcs) {
            objTable[symTab.insert(pair.first)] = pair.second;
        }
    }

    ModuleBuilder &mod() { return builder; }
private:
    std::string startFunctionName;
    std::string funcCurrent;
    ModuleBuilder builder;

    // Symbol table uses IDs and Objects
    Object look(const std::string &name);
    void define(const std::string &name, Object object);
    void redefine(const std::string &name, Object object);

    SymbolTable   symTab;
    std::map<SymbolTable::ID, Object> objTable;


    // AST Numbering algorithm for Phi-node generation
    void          writeVariable(SymbolTable::ID, llvm::BasicBlock*, llvm::Value *);
    llvm::Value*  readVariable(SymbolTable::ID variable, llvm::BasicBlock* block);
    llvm::Value*  readVariableRecursive(SymbolTable::ID, llvm::BasicBlock* );
    void          sealBlock(llvm::BasicBlock *);
    llvm::PHINode *newPhi(llvm::BasicBlock* );
    void          addPhiOperands(SymbolTable::ID, llvm::PHINode *);

    std::map<SymbolTable::ID, std::map<llvm::BasicBlock*, llvm::Value*>> currentDefs;
    std::map<llvm::BasicBlock*, std::map<SymbolTable::ID, llvm::PHINode*>> incompletePhis;
    std::set<llvm::BasicBlock*> sealedBlocks;
};
