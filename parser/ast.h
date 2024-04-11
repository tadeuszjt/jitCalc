#pragma once

#include <memory>
#include <variant>
#include <iostream>
#include <cassert>
#include <vector>

// defines the AST as several types. Provides the << operator to print to std::cout.
namespace ast {


struct Node {
    virtual ~Node() { }
};

struct Expr : public Node {
    virtual ~Expr() {}
};

struct Stmt : public Node {
    virtual ~Stmt() {}
};


struct Ident : public Node {
    Ident(const std::string &ident) : ident(ident) {}
    ~Ident() override {}
    const std::string ident;
};

struct IdentList : public Node {
    IdentList() {}
    void cons(std::shared_ptr<Ident> ident) { identList.insert(identList.begin(), ident); }
    std::vector<std::shared_ptr<Ident>> identList;
    ~IdentList() override {}
};


struct Operator : public Node {
    Operator(char symbol) : symbol(symbol) {}
    const char symbol;

    ~Operator() override {}
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
    Operator symbol;

    Infix(const std::shared_ptr<Expr> left, Operator symbol, const std::shared_ptr<Expr> right) :
        left(left),
        right(right),
        symbol(symbol)
    {}
    ~Infix() override {}
};

struct Prefix : public Expr {
    std::shared_ptr<Expr> right;
    Operator symbol;

    Prefix(Operator symbol, std::shared_ptr<Expr> right) : right(right), symbol(symbol) {}
    ~Prefix() override {}
};

struct Call : public Expr {
    std::string name;
    Call(const std::string& name) : name(name) {}
    ~Call() override {}
};

struct FnDef : public Stmt {
    FnDef(const std::shared_ptr<Ident> name,
          const std::shared_ptr<IdentList> args,
          const std::shared_ptr<Expr> body)
            : name(name), args(args), body(body) {}

    const std::shared_ptr<Ident> name;
    const std::shared_ptr<IdentList> args;
    const std::shared_ptr<Expr> body;

    ~FnDef() override {}
};


}

