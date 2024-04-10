#include "lexer.h"
#include <cctype>
#include <cassert>

// this class is used to store stack of indent spaces during lexing.
class Lexer {
public:
    Lexer(std::string &str) : start(str.begin()), end(str.end()) {
        indentStack.push_back("");
    }

    std::vector<Token> lexTokens() {
        std::vector<Token> tokens;

        for (;;) {
            skipSpace();
            auto newline = lexNewline(start);
            if (newline.size() > 0) {
                for (TokenSpace space : newline) {
                    tokens.push_back(space);
                }
            }


            auto result = lexAny();
            if (!result.has_value()) {
                break;
            }

            tokens.push_back(result.value().token);
            start = result.value().rest;
        }

        if (start == end) {
            return tokens;
        } else {
            start = end;
            return {};
        }
    }

private:
    struct LexSuccess {
        std::string::iterator rest;
        Token token;
        LexSuccess(std::string::iterator it, Token tok) : rest(it), token(tok) {};
    };
    using LexResult = std::optional<LexSuccess>;

    void skipSpace() {
        for (; start != end && (*start == '\t' || *start == ' '); start++) {
        }
    }

    LexResult lexAny() {
        auto floating = lexFloating();
        if (floating.has_value()) {
            return floating;
        }

        auto integer = lexInteger();
        if (integer.has_value()) {
            return integer;
        }

        auto symbol = lexSymbol();
        if (symbol.has_value()) {
            return symbol;
        }

        auto keyword = lexKeyword();
        if (keyword.has_value()) {
            return keyword;
        }

        auto ident = lexIdent();
        if (ident.has_value()) {
            return ident;
        }

        return std::nullopt;
    }

    // lexNewline implements whitespace-specific syntax, it will return either:
    //  SPACE_INDENT                    -- indent level has increased by 1
    //  SPACE_NEWLINE                   -- indent level stays the same
    //  {SPACE_DEDENT, SPACE_DEDENT...} -- dedent level has decreased by number of tokens
    std::vector<TokenSpace> lexNewline(std::string::iterator &out) {
        std::string::iterator it = start;
        if (it == end || *it != '\n') {
            return {};
        }
        it++;
        std::string::iterator spacesStart = it;

        for (; it != end && (*it == '\t' || *it == ' '); it++) {
        }

        out = it;
        std::string spaces = std::string(spacesStart, it);

        if (spaces == indentStack.back()) {
            return {SPACE_NEWLINE};
        } else if (spaces.substr(0, indentStack.back().size()) == indentStack.back()) {
            indentStack.push_back(spaces);
            return {SPACE_INDENT};
        } else {
            std::vector<TokenSpace> tokens;
            for (; spaces != indentStack.back(); indentStack.pop_back()) {
                assert(indentStack.size() > 0);
                tokens.push_back(SPACE_DEDENT);
            }

            assert(spaces == indentStack.back());
            return tokens;
        }
    }

    LexResult lexKeyword() {
        auto result = lexIdent();
        if (!result.has_value()) {
            return std::nullopt;
        }

        auto ident = std::get<TokenIdent>(result.value().token);

        for (const std::string &keyword : keywords) {
            if (ident.str == keyword) {
                return LexSuccess(result.value().rest, TokenKeyword(ident.str));
            }
        }
        return std::nullopt;
    }

    LexResult lexInteger() {
        std::string::iterator it;
        for (it = start; it != end && isdigit(*it); it++) {
        }
        if (it != start) {
            return LexSuccess(it, TokenInt{std::stoi(std::string(start, it))});
        }
        return std::nullopt;
    }

    LexResult lexFloating() {
        std::string::iterator it = start;
        if (it == end || !isdigit(*it)) {
            return std::nullopt;
        }
        for (; it != end && isdigit(*it); it++) {
        }
        if (it == end || *it != '.') {
            return std::nullopt;
        }
        it++;
        for (; it != end && isdigit(*it); it++) {
        }
        return LexSuccess(it, TokenFloat{std::stod(std::string(start, it))});
    }

    LexResult lexIdent() {
        std::string::iterator it = start;
        if (it != end && isalpha(*it)) {
            it++;
        }
        for (; it != end && (isalpha(*it) || isdigit(*it)); it++) {
        }
        if (it != start) {
            return LexSuccess(it, TokenIdent{std::string(start, it)});
        }
        return std::nullopt;
    }


    LexResult lexSymbol() {
        std::string::iterator it = start;

        std::string symbols = "+-*/()";

        if (it != end && symbols.find(*it) != std::string::npos) {
            it++;
        }

        if (it != start) {
            return LexSuccess(it, TokenSymbol{*(it - 1)});
        }

        return std::nullopt;
    }

    std::string::iterator start, end;
    std::vector<std::string> indentStack;
};


std::ostream& operator<<(std::ostream& os, const Token& token) {
    if (std::holds_alternative<TokenInt>(token)) {
        os << "TokenInt(" << std::get<TokenInt>(token) << ")";
    } else if (std::holds_alternative<TokenIdent>(token)) {
        os << "TokenIdent(" << std::get<TokenIdent>(token).str << ")";
    } else if (std::holds_alternative<TokenKeyword>(token)) {
        os << "TokenKeyword(" << std::get<TokenKeyword>(token).str << ")";
    } else if (std::holds_alternative<TokenSymbol>(token)) {
        os << "TokenSymbol(" << std::get<TokenSymbol>(token) << ")";
    } else if (std::holds_alternative<TokenSpace>(token)) {
        switch (std::get<TokenSpace>(token)) {
        case SPACE_NEWLINE: os << "TokenSpace(NEWLINE)"; break;
        case SPACE_INDENT: os << "TokenSpace(INDENT)"; break;
        case SPACE_DEDENT: os << "TokenSpace(DEDENT)"; break;
        default: assert(false); break;
        }
    } else {
        assert(false);
    }
    return os;
}

std::vector<Token> lexTokens(std::string& str) {
    Lexer lexer(str);
    return lexer.lexTokens();
}
