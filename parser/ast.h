#pragma once

#include <memory>
#include <variant>
#include <iostream>
#include <cassert>

// defines the AST as several types. Provides the << operator to print to std::cout.
namespace ast {

struct EExpression;

using Symbol = char;
using Integer = int;

struct Infix {
    std::unique_ptr<EExpression> left, right;
    Symbol symbol;

    Infix(const Infix& infix) :
        left(std::make_unique<EExpression>(*infix.left)),
        right(std::make_unique<EExpression>(*infix.right)),
        symbol(infix.symbol)
    {}

    Infix(const EExpression& left, Symbol symbol, const EExpression& right) :
        left(std::make_unique<EExpression>(left)),
        right(std::make_unique<EExpression>(right)),
        symbol(symbol)
    {}
};

struct Prefix {
    std::unique_ptr<EExpression> right;
    Symbol symbol;

    Prefix(const Prefix& prefix) :
        right(std::make_unique<EExpression>(*prefix.right)),
        symbol(prefix.symbol)
    {}

    Prefix(Symbol symbol, const EExpression& right) :
        right(std::make_unique<EExpression>(right)),
        symbol(symbol)
    {}
};


struct EExpression {
    using Type = enum {
        TYPE_INFIX,
        TYPE_PREFIX,
        TYPE_INTEGER
    };

    EExpression(const Infix &x) : t(TYPE_INFIX), variant(x) { }
    EExpression(const Prefix &x) : t(TYPE_PREFIX), variant(x) { }
    EExpression(const Integer &x) : t(TYPE_INTEGER), variant(x) { }
    EExpression(const EExpression &x) : t(x.type()), variant(x.variant) {}

    Type type() const { return t; }
    const Infix& getInfix() const { return std::get<Infix>(variant); }
    const Prefix& getPrefix() const { return std::get<Prefix>(variant); }
    const Integer& getInteger() const { return std::get<Integer>(variant); }

private:
    Type t;
    std::variant<Infix, Prefix, Integer> variant; 
};

}
//
//std::ostream& operator<<(std::ostream& os, const ast::Expression& expression);
