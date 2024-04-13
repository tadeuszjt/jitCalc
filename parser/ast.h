#pragma once

#include <memory>
#include <variant>
#include <iostream>
#include <cassert>
#include <vector>

namespace ast {

enum Operator {
    Plus,
    Minus,
    Times,
    Divide,
    LT,
    GT,
    EqEq
};

struct Stmt;

struct Node {
    virtual ~Node() { }
};

struct Expr : public Node {
    virtual ~Expr() {}
};

struct Stmt : public Node {
    virtual ~Stmt() {}
};


struct Return : public Stmt {
    virtual ~Return() override {}
    Return(const std::shared_ptr<Expr> expr) : expr(expr) {}
    const std::shared_ptr<Expr> expr;
};


struct Ident : public Expr {
    Ident(const std::string &ident) : ident(ident) {}
    ~Ident() override {}
    const std::string ident;
};


struct StmtList : public Node {
    StmtList() {}
    ~StmtList() override {}
    void cons(std::shared_ptr<Stmt> stmt) { stmtList.insert(stmtList.begin(), stmt); }
    std::vector<std::shared_ptr<Stmt>> stmtList;
};


struct IdentList : public Node {
    IdentList() {}
    void cons(std::shared_ptr<Ident> ident) { identList.insert(identList.begin(), ident); }
    size_t size() { return identList.size(); }
    std::vector<std::shared_ptr<Ident>> identList;
    ~IdentList() override {}
};

struct ExprList : public Node {
    ExprList() {}
    void cons(std::shared_ptr<Expr> expr) { exprList.insert(exprList.begin(), expr); }
    size_t size() { return exprList.size(); }
    std::vector<std::shared_ptr<Expr>> exprList;
    ~ExprList() override {}
};

struct Integer : public Expr {
    Integer(int integer) : integer(integer) {}
    const int integer;
    ~Integer() override {}
};

struct Floating : public Expr {
    Floating(double floating) : floating(floating) {}
    const double floating;
    ~Floating() override {}
};

struct Infix : public Expr {
    const std::shared_ptr<Expr> left, right;
    Operator op;

    Infix(const std::shared_ptr<Expr> left, Operator op, const std::shared_ptr<Expr> right) :
        left(left),
        right(right),
        op(op)
    {}
    ~Infix() override {}
};

struct Prefix : public Expr {
    std::shared_ptr<Expr> right;
    Operator op;

    Prefix(Operator op, std::shared_ptr<Expr> right) : right(right), op(op) {}
    ~Prefix() override {}
};

struct Call : public Expr {
    Call(const std::string& name, const std::shared_ptr<ExprList> args)
        : name(name), args(args) {}
    ~Call() override {}

    std::string                     name; 
    const std::shared_ptr<ExprList> args;
};

struct FnDef : public Stmt {
    FnDef(const std::shared_ptr<Ident> name,
          const std::shared_ptr<IdentList> args,
          const std::shared_ptr<StmtList> body)
            : name(name), args(args), body(body) {}

    const std::shared_ptr<Ident> name;
    const std::shared_ptr<IdentList> args;
    const std::shared_ptr<StmtList> body;

    ~FnDef() override {}
};


struct If : public Stmt {
    If(const std::shared_ptr<Expr> cnd,
       const std::shared_ptr<StmtList> body)
            : cnd(cnd), body(body) {}

    const std::shared_ptr<Expr> cnd;
    const std::shared_ptr<StmtList> body;

    ~If() override {}
};



}

