#pragma once

#include <memory>
#include <variant>
#include <iostream>
#include <cassert>
#include <vector>

#include "sparse.h"
#include "lexer.h"

namespace ast {

enum Operator { Plus, Minus, Times, Divide, LT, GT, EqEq };

struct List;
struct Program;
struct Integer;
struct Prefix;
struct Infix;
struct Return;
struct Ident;
struct Call;
struct FnDef;
struct If;
struct Let;
struct Set;
struct For;

using Node = std::variant<Program, List, Integer, Prefix, Infix, Return, Ident, Call, FnDef, If,
    Let, Set, For>;

// a class to represent a list of nodes such as a comma-separated list of expressions.
struct List {
public:
    List& operator=(const List&) = default;

    List(TextPos pos) : pos(pos) {}
    List(TextPos pos, Sparse<Node>::Key item) : pos(pos) { list.push_back(item); }

    void cons(Sparse<Node>::Key item) { list.insert(list.begin(), item); }
    size_t size() { return list.size(); }

    std::vector< Sparse<Node>::Key > list;
    TextPos pos;
};


struct Program {
    Program& operator=(const Program&) = default;

    Program(TextPos pos, Sparse<ast::Node>::Key key) : pos(pos), stmtList(key) {}
    Sparse<Node>::Key stmtList;
    TextPos pos;
};


struct Integer {
    Integer& operator=(const Integer&) = default;

    Integer(TextPos pos, int integer) : pos(pos), integer(integer) {}

    int integer;
    TextPos pos;
};

struct Prefix {
    Prefix& operator=(const Prefix&) = default;

    Prefix(TextPos pos, Operator op, Sparse<Node>::Key right) : pos(pos), right(right), op(op) {}

    Sparse<Node>::Key right;
    Operator op;
    TextPos pos;
};

struct Infix {
    Infix& operator=(const Infix&) = default;

    Infix(TextPos pos, Sparse<Node>::Key left, Operator op, Sparse<Node>::Key right) :
        pos(pos),
        left(left),
        right(right),
        op(op)
    {}

    Sparse<Node>::Key left, right;
    Operator op;
    TextPos pos;
};


struct Return {
    Return& operator=(const Return&) = default;

    Return(TextPos pos, Sparse<Node>::Key expr) : pos(pos), expr(expr) {}
    Sparse<Node>::Key expr;
    TextPos pos;
};


struct Ident {
    Ident& operator=(const Ident&) = default;

    Ident(TextPos pos, const std::string &ident) : pos(pos), ident(ident) {}
    std::string ident;
    TextPos pos;
};


struct Call {
    Call& operator=(const Call&) = default;

    Call(TextPos pos, std::string& name, Sparse<Node>::Key args)
        : pos(pos), name(name), args(args) {}

    std::string name; 
    Sparse<Node>::Key args;
    TextPos pos;
};


struct FnDef {
    FnDef& operator=(const FnDef&) = default;

    FnDef(TextPos pos, const std::string &name, Sparse<Node>::Key args, Sparse<Node>::Key body)
            : pos(pos), name(name), args(args), body(body) {}

    std::string name;
    Sparse<Node>::Key args, body;
    TextPos pos;
};


struct If {
    If& operator=(const If&) = default;

    If(TextPos pos, Sparse<Node>::Key cnd, Sparse<Node>::Key trueBody, Sparse<Node>::Key falseBody)
        : pos(pos), cnd(cnd), trueBody(trueBody), falseBody(falseBody) {}

    Sparse<Node>::Key cnd, trueBody, falseBody;
    TextPos pos;
};


struct Let {
    Let& operator=(const Let&) = default;

    Let(TextPos pos, std::string &name, Sparse<Node>::Key expr)
        : pos(pos), name(name), expr(expr) {}

    std::string name;
    Sparse<Node>::Key expr;
    TextPos pos;
};


struct Set {
    Set& operator=(const Set&) = default;

    Set(TextPos pos, std::string &name, Sparse<Node>::Key expr)
        : pos(pos), name(name), expr(expr) {}

    std::string name;
    Sparse<Node>::Key expr;
    TextPos pos;
};


struct For {
    For& operator=(const For&) = default;

    For(TextPos pos, Sparse<Node>::Key cnd, Sparse<Node>::Key body)
        : pos(pos), cnd(cnd), body(body) {}

    Sparse<Node>::Key cnd, body;
    TextPos pos;
};
}

