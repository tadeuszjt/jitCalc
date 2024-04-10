#include "parse.h"
#include "lexer.h"
#include "grammar.tab.hh"

#include <cassert>

static std::vector<Token> tokens;
static int tokenIndex;

extern ast::Program *bisonProgramResult;

int yylex(yy::parser::semantic_type *un) {
    if (tokenIndex >= tokens.size()) {
        return yy::parser::token::YYEOF;
    }

    auto token = tokens[tokenIndex++];

    if (std::holds_alternative<TokenInt>(token)) {
        un->integer = std::get<TokenInt>(token);
        return yy::parser::token::INTEGER;
    }

    if (std::holds_alternative<TokenFloat>(token)) {
        un->floating = std::get<TokenFloat>(token);
        return yy::parser::token::FLOATING;
    }

    if (std::holds_alternative<TokenSymbol>(token)) {
        return std::get<TokenSymbol>(token);
    }

    if (std::holds_alternative<TokenIdent>(token)) {
        un->ident = new std::string(std::get<TokenIdent>(token).str);
        return yy::parser::token::IDENT;
    }

    if (std::holds_alternative<TokenKeyword>(token)) {
        auto keyword = std::string(std::get<TokenKeyword>(token).str);
        if (keyword == "fn") {
            return yy::parser::token::KW_FN;
        }

        assert(false);
    }

    if (std::holds_alternative<TokenSpace>(token)) {
        switch (std::get<TokenSpace>(token)) {
        case SPACE_NEWLINE: return yy::parser::token::NEWLINE;
        case SPACE_INDENT: return yy::parser::token::INDENT;
        case SPACE_DEDENT: return yy::parser::token::DEDENT;
        default: assert(false); break;
        }
    }

    assert(false);
}

ast::Program parse(std::string& text) {
    tokens = lexTokens(text);
    tokenIndex = 0;

    yy::parser parser;
    parser.parse();

    if (bisonProgramResult != nullptr) {
        auto result = *bisonProgramResult;
        delete bisonProgramResult;
        bisonProgramResult = nullptr;
        return result;
    } else {
        assert(false);
    }
}
