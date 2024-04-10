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
    using Variant = std::variant<Infix, Prefix, Integer, Floating, Call>;
    Expr(const Variant& x) : variant(x) {}
    Expr(const Expr &x) : variant(x.variant) {}

    const Infix& getInfix() const { return std::get<Infix>(variant); }
    const Prefix& getPrefix() const { return std::get<Prefix>(variant); }
    const Integer& getInteger() const { return std::get<Integer>(variant); }
    const Floating& getFloating() const { return std::get<Floating>(variant); }
    const Call& getCall() const { return std::get<Call>(variant); }

    bool hasInfix() const { return std::holds_alternative<Infix>(variant); }
    bool hasPrefix() const { return std::holds_alternative<Prefix>(variant); }
    bool hasInteger() const { return std::holds_alternative<Integer>(variant); }
    bool hasFloating() const { return std::holds_alternative<Floating>(variant); }
    bool hasCall() const { return std::holds_alternative<Call>(variant); }
private:
    Variant variant;
};



struct FnDef {
    FnDef() {}
};

using Stmt = std::variant<FnDef>; 



using Program = std::variant<Expr, Stmt>;


}

