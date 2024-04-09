#pragma once

#include <memory>
#include <variant>
#include <iostream>
#include <cassert>

// defines the AST as several types. Provides the << operator to print to std::cout.
namespace ast {

struct Expr;

using Symbol = char;
using Integer = int;
using Floating = double;

struct Infix {
    std::unique_ptr<Expr> left, right;
    Symbol symbol;

    Infix(const Infix& infix) :
        left(std::make_unique<Expr>(*infix.left)),
        right(std::make_unique<Expr>(*infix.right)),
        symbol(infix.symbol)
    {}

    Infix(const Expr& left, Symbol symbol, const Expr& right) :
        left(std::make_unique<Expr>(left)),
        right(std::make_unique<Expr>(right)),
        symbol(symbol)
    {}
};

struct Prefix {
    std::unique_ptr<Expr> right;
    Symbol symbol;

    Prefix(const Prefix& prefix) :
        right(std::make_unique<Expr>(*prefix.right)),
        symbol(prefix.symbol)
    {}

    Prefix(Symbol symbol, const Expr& right) :
        right(std::make_unique<Expr>(right)),
        symbol(symbol)
    {}
};

struct Call {
    std::string name;

    Call(const Call& call) : name(call.name) {}
    Call(const std::string& name) : name(name) {}
};


struct Expr {
    using Type = enum {
        TYPE_INFIX,
        TYPE_PREFIX,
        TYPE_INTEGER,
        TYPE_FLOATING,
        TYPE_CALL
    };

    Expr(const Infix &x) : t(TYPE_INFIX), variant(x) { }
    Expr(const Prefix &x) : t(TYPE_PREFIX), variant(x) { }
    Expr(const Integer &x) : t(TYPE_INTEGER), variant(x) { }
    Expr(const Floating &x) : t(TYPE_FLOATING), variant(x) { }
    Expr(const Call &x) : t(TYPE_CALL), variant(x) { }
    Expr(const Expr &x) : t(x.type()), variant(x.variant) {}

    Type type() const { return t; }
    const Infix& getInfix() const { return std::get<Infix>(variant); }
    const Prefix& getPrefix() const { return std::get<Prefix>(variant); }
    const Integer& getInteger() const { return std::get<Integer>(variant); }
    const Floating& getFloating() const { return std::get<Floating>(variant); }
    const Call& getCall() const { return std::get<Call>(variant); }

private:
    Type t;
    std::variant<Infix, Prefix, Integer, Floating, Call> variant; 
};

}
//
//std::ostream& operator<<(std::ostream& os, const ast::Expression& expression);
