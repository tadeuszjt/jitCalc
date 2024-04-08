#pragma once

#include <memory>
#include <variant>
#include <iostream>

// defines the AST as several types. Provides the << operator to print to std::cout.
namespace ast {
struct Infix;

using Symbol = char;
using Integer = int;
using Expression = std::variant<Infix, Integer, Symbol>;

struct Infix {
    std::unique_ptr<Expression> left, right;
    Symbol symbol;

    Infix(const Infix& infix) :
        left(std::make_unique<Expression>(*infix.left)),
        right(std::make_unique<Expression>(*infix.right)),
        symbol(infix.symbol)
    {}

    Infix(const Expression& left, Symbol symbol, const Expression& right) :
        left(std::make_unique<Expression>(left)),
        right(std::make_unique<Expression>(right)),
        symbol(symbol)
    {}
};

}

std::ostream& operator<<(std::ostream& os, const ast::Expression& expression);
