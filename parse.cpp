#include "parse.h"
#include "lexer.h"
#include "grammar.tab.hh"

#include <cassert>
#include <memory>

static std::vector<Token> tokens;
static std::string        source;
static int tokenIndex;

extern ast::Node *bisonProgramResult;

std::string tokenString(Token &tok) {
    return source.substr(tok.getStart(), tok.getEnd() - tok.getStart());
}

int yylex(yy::parser::semantic_type *un) {
    if (tokenIndex >= tokens.size()) {
        return yy::parser::token::YYEOF;
    }

    auto token = tokens[tokenIndex++];

    switch (token.getKind()) {
    case Token::KindInt: {
            *un =  new ast::Integer(std::stoi(tokenString(token)));
            return yy::parser::token::INTEGER;
        }

    case Token::KindFloat: {
            *un = new ast::Floating(std::stod(tokenString(token)));
            return yy::parser::token::FLOATING;
        }

    case Token::KindSymbol: {
            auto symbol = tokenString(token);
            if (symbol == "+") { return '+'; }
            if (symbol == "-") { return '-'; }
            if (symbol == "*") { return '*'; }
            if (symbol == "/") { return '/'; }
            if (symbol == "(") { return '('; }
            if (symbol == ")") { return ')'; }
            if (symbol == "<") { return '<'; }
            if (symbol == ">") { return '>'; }
            if (symbol == "==") { return yy::parser::token::EqEq; }

            std::cerr << "symbol was: " << symbol << std::endl;
            assert(false);
            break;
        }

    case Token::KindIdent: {
            *un = new ast::Ident(tokenString(token));
            return yy::parser::token::ident;
        }

    case Token::KindKeyword: {
            auto keyword = tokenString(token);
            if (keyword == "fn") {
                return yy::parser::token::fn;
            }
            if (keyword == "if") {
                return yy::parser::token::If;
            }
            if (keyword == "else") {
                return yy::parser::token::Else;
            }
            if (keyword == "return") {
                return yy::parser::token::Return;
            }

            assert(false);
        }

    case Token::KindNewline: {
            return yy::parser::token::NEWLINE;
        }
    case Token::KindIndent: {
            return yy::parser::token::INDENT;
        }
    case Token::KindDedent: {
            return yy::parser::token::DEDENT;
        }
    default:
        assert(false);
        break;
    }

    assert(false);
    return 0;
}

ast::Node *parse(std::string& text) {
    Lexer lexer;
    tokens = lexer.lexTokens(text);
    source = text;
    tokenIndex = 0;

    yy::parser parser;
    parser.parse();

    if (bisonProgramResult) {
        return bisonProgramResult;
    } else {
        assert(false);
    }
}
