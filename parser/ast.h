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


struct Ident : public Node {
    Ident(const std::string &ident) : ident(ident) {}
    ~Ident() override {}
    const std::string ident;
};

struct Operator : public Node {
    Operator(char op) : op(op) {}
    ~Operator() override {}
    char op;
};


struct Symbol : public Node {
    Symbol(char symbol) : symbol(symbol) {}
    const char symbol;

    ~Symbol() override {}
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
    Symbol symbol;

    Infix(const std::shared_ptr<Expr> left, Symbol symbol, const std::shared_ptr<Expr> right) :
        left(left),
        right(right),
        symbol(symbol)
    {}
    ~Infix() override {}
};

struct Prefix : public Expr {
    std::shared_ptr<Expr> right;
    Symbol symbol;

    Prefix(Symbol symbol, std::shared_ptr<Expr> right) : right(right), symbol(symbol) {}
    ~Prefix() override {}
};

struct Call : public Expr {
    std::string name;
    Call(const Call& call) : name(call.name) {}
    Call(const std::string& name) : name(name) {}
    ~Call() override {}
};

struct FnDef : public Node {
    FnDef(const std::string &name, const std::vector<std::string> argList, const Expr& body)
        : name(name), body(body), argList(argList) {}

    const std::string name;
    const Expr body;
    const std::vector<std::string> argList;
    ~FnDef() override {}
};


}

