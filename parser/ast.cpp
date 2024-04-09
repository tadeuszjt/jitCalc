#include "ast.h"
#include <cassert>

using namespace ast;

//std::ostream& operator<<(std::ostream& os, const Expression& expression) {
//    if (std::holds_alternative<Integer>(expression)) {
//        os << "Integer(" << std::get<Integer>(expression) << ")";
//    } else if (std::holds_alternative<Symbol>(expression)) {
//        os << "Symbol(" << std::get<Symbol>(expression) << ")";
//    } else if (std::holds_alternative<Infix>(expression)) {
//        auto infix = std::get<ast::Infix>(expression);
//        os << "Infix(" << *infix.left << " " << infix.symbol << " " << *infix.right << ")";
////    } else if (std::holds_alternative<Prefix>(expression)) {
////        auto prefix = std::get<ast::Prefix>(expression);
////        os << "Prefix(" << infix.symbol << *infix.right << ")";
//    } else {
//        assert(false);
//    }
//    return os;
//}
//
