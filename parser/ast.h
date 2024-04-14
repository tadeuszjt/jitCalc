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


struct Node {
    virtual ~Node() { }
};

struct Expr : public Node {
    virtual ~Expr() {}
};

struct Stmt : public Node {
    virtual ~Stmt() {}
};


// a class to represent a list of nodes such as a comma-separated list of expressions.
template <typename T>
class List : public Node {
public:
    List() {}
    ~List() override {}

    List(std::shared_ptr<T> item) { list.push_back(item); }
    void cons(std::shared_ptr<T> item) { list.insert(list.begin(), item); }
    size_t size() { return list.size(); }

    std::vector<std::shared_ptr<T>> list;
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
    Call(const std::string& name, const std::shared_ptr<List<Expr>> args)
        : name(name), args(args) {}
    ~Call() override {}

    std::string                     name; 
    const std::shared_ptr<List<Expr>> args;
};

struct FnDef : public Stmt {
    FnDef(const std::shared_ptr<Ident> name,
          const std::shared_ptr<List<Ident>> args,
          const std::shared_ptr<List<Stmt>> body)
            : name(name), args(args), body(body) {}

    const std::shared_ptr<Ident> name;
    const std::shared_ptr<List<Ident>> args;
    const std::shared_ptr<List<Stmt>> body;

    ~FnDef() override {}
};


struct If : public Stmt {
    If(const std::shared_ptr<Expr> cnd,
       const std::shared_ptr<List<Stmt>> body)
            : cnd(cnd), body(body) {}

    const std::shared_ptr<Expr> cnd;
    const std::shared_ptr<List<Stmt>> body;

    ~If() override {}
};



}

