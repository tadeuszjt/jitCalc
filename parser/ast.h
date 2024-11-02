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


struct Return : public Node {
    virtual ~Return() override {}
    Return(const std::shared_ptr<Node> expr) : expr(expr) {}
    const std::shared_ptr<Node> expr;
};


struct Ident : public Node {
    Ident(const std::string &ident) : ident(ident) {}
    ~Ident() override {}
    const std::string ident;
};


struct Integer : public Node {
    Integer(int integer) : integer(integer) {}
    const int integer;
    ~Integer() override {}
};

struct Floating : public Node {
    Floating(double floating) : floating(floating) {}
    const double floating;
    ~Floating() override {}
};

struct Infix : public Node {
    const std::shared_ptr<Node> left, right;
    Operator op;

    Infix(const std::shared_ptr<Node> left, Operator op, const std::shared_ptr<Node> right) :
        left(left),
        right(right),
        op(op)
    {}
    ~Infix() override {}
};

struct Prefix : public Node {
    std::shared_ptr<Node> right;
    Operator op;

    Prefix(Operator op, std::shared_ptr<Node> right) : right(right), op(op) {}
    ~Prefix() override {}
};

struct Call : public Node {
    Call(const std::string& name, const std::shared_ptr<List<Node>> args)
        : name(name), args(args) {}
    ~Call() override {}

    std::string                     name; 
    const std::shared_ptr<List<Node>> args;
};

struct FnDef : public Node {
    FnDef(const std::shared_ptr<Ident> name,
          const std::shared_ptr<List<Ident>> args,
          const std::shared_ptr<List<Node>> body)
            : name(name), args(args), body(body) {}

    const std::shared_ptr<Ident> name;
    const std::shared_ptr<List<Ident>> args;
    const std::shared_ptr<List<Node>> body;

    ~FnDef() override {}
};


struct If : public Node {
    If(const std::shared_ptr<Node> cnd,
       const std::shared_ptr<List<Node>> body)
            : cnd(cnd), body(body) {}

    const std::shared_ptr<Node> cnd;
    const std::shared_ptr<List<Node>> body;

    ~If() override {}
};



}

