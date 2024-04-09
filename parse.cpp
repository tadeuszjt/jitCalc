#include "parse.h"
#include "lexer.h"
#include "grammar.tab.hh"

#include <cassert>

static std::vector<Token> tokens;
static int tokenIndex;

extern ast::EExpression *bisonParseResult;

int yylex(yy::parser::semantic_type *un) {
    if (tokenIndex >= tokens.size()) {
        return yy::parser::token::YYEOF;
    }

    auto token = tokens[tokenIndex++];

    if (std::holds_alternative<TokenInt>(token)) {
        un->integer = std::get<TokenInt>(token);
        return yy::parser::token::INTEGER;
    } else if (std::holds_alternative<TokenSymbol>(token)) {
        return std::get<TokenSymbol>(token);
    }
    assert (false);
}

ast::EExpression parse(std::string& text) {
    tokens = lexTokens(text);
    tokenIndex = 0;

    yy::parser parser;
    parser.parse();

    assert(bisonParseResult != nullptr);
    auto result = *bisonParseResult;
    delete bisonParseResult;
    bisonParseResult = nullptr;
    return result;
}
