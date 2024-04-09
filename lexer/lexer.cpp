#include "lexer.h"
#include <cctype>
#include <cassert>


std::ostream& operator<<(std::ostream& os, const Token& token) {
    if (std::holds_alternative<TokenInt>(token)) {
        os << "TokenInt(" << std::get<TokenInt>(token) << ")";
    } else if (std::holds_alternative<TokenIdent>(token)) {
        os << "TokenIdent(" << std::get<TokenIdent>(token) << ")";
    } else if (std::holds_alternative<TokenSymbol>(token)) {
        os << "TokenSymbol(" << std::get<TokenSymbol>(token) << ")";
    } else {
        assert(false);
    }
    return os;
}

std::vector<Token> lexTokens(std::string& str) {
    auto it = str.begin();
    std::vector<Token> tokens;

    while (true) {
        for (; it != str.end() && isspace(*it); it++) {
        }

        auto result = lexAny(it, str.end());
        if (!result.has_value()) {
            break;
        }

        tokens.push_back(result.value().token);
        it = result.value().rest;
    }

    if (it == str.end()) {
        return tokens;
    } else {
        return {};
    }
}

LexResult lexInteger(std::string::iterator begin, std::string::iterator end) {
    std::string::iterator it;
    for (it = begin; it != end && isdigit(*it); it++) {
    }

    if (it != begin) {
        return LexSuccess(it, TokenInt{std::stoi(std::string(begin, it))});
    }

    return std::nullopt;
}

LexResult lexFloating(std::string::iterator begin, std::string::iterator end) {
    std::string::iterator it = begin;

    // must start with digit
    if (it == end || !isdigit(*it)) {
        return std::nullopt;
    }

    // consume all digits
    for (; it != end && isdigit(*it); it++) {
    }

    // consume '.'
    if (it == end || *it != '.') {
        return std::nullopt;
    }
    it++;

    // consume any more digits
    for (; it != end && isdigit(*it); it++) {
    }

    return LexSuccess(it, TokenFloat{std::stod(std::string(begin, it))});
}

LexResult lexIdent(std::string::iterator begin, std::string::iterator end) {
    std::string::iterator it = begin;

    if (it != end && isalpha(*it)) {
        it++;
    }

    for (; it != end && (isalpha(*it) || isdigit(*it)); it++) {
    }

    if (it != begin) {
        return LexSuccess(it, TokenIdent{std::string(begin, it)});
    }

    return std::nullopt;
}


LexResult lexSymbol(std::string::iterator begin, std::string::iterator end) {
    std::string::iterator it = begin;

    std::string symbols = "+-*/()";

    if (it != end && symbols.find(*it) != std::string::npos) {
        it++;
    }

    if (it != begin) {
        return LexSuccess(it, TokenSymbol{*(it - 1)});
    }

    return std::nullopt;
}


LexResult lexAny(std::string::iterator begin, std::string::iterator end) {
    auto floating = lexFloating(begin, end);
    if (floating.has_value()) {
        return floating;
    }

    auto integer = lexInteger(begin, end);
    if (integer.has_value()) {
        return integer;
    }

    auto symbol = lexSymbol(begin, end);
    if (symbol.has_value()) {
        return symbol;
    }

    auto ident = lexIdent(begin, end);
    if (ident.has_value()) {
        return ident;
    }

    return std::nullopt;
}
