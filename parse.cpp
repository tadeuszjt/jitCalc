#include "parse.h"
#include "lexer.h"
#include "grammar.tab.hh"

#include <cassert>
#include <memory>

#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>

static std::string            source;
static std::unique_ptr<Lexer> lexer;
extern ast::Node              *bisonProgramResult;


std::string tokenString(Token &tok) {
    return source.substr(tok.getStart(), tok.getEnd() - tok.getStart());
}

int yylex(yy::parser::semantic_type *un) {
    auto token = lexer->nextToken();

    switch (token.getKind()) {

    case Token::KindEOF:
        return yy::parser::token::YYEOF;
    case Token::KindNewline:
        return yy::parser::token::NEWLINE;
    case Token::KindIndent:
        return yy::parser::token::INDENT;
    case Token::KindDedent:
        return yy::parser::token::DEDENT;

    case Token::KindInt:
        *un =  new ast::Integer(std::stoi(tokenString(token)));
        return yy::parser::token::INTEGER;

    case Token::KindFloat:
        *un = new ast::Floating(std::stod(tokenString(token)));
        return yy::parser::token::FLOATING;

    case Token::KindIdent:
        *un = new ast::Ident(tokenString(token));
        return yy::parser::token::ident;

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

            llvm::errs() << "why: " << keyword << '\n';

            assert(false);
        }
        break;

    default:
        assert(false);
        break;
    }

    assert(false);
    return 0;
}

ast::Node *parse(std::string& text) {
    bisonProgramResult = nullptr;
    source = text;
    lexer = std::make_unique<Lexer>(source);

    std::unique_ptr<llvm::MemoryBuffer> buffer = llvm::MemoryBuffer::getMemBufferCopy(source, "buffer");
    assert(buffer);
    llvm::SourceMgr sourceMgr;
    int bufId = sourceMgr.AddNewSourceBuffer(std::move(buffer), llvm::SMLoc());

    int errorCount = 0;
    for (;;) {
        auto token = lexer->nextToken();
        if (token.getKind() == Token::KindEOF) {
            break;
        }
        if (token.getKind() == Token::KindInvalid) {
            errorCount++;
            llvm::SMLoc loc = llvm::SMLoc::getFromPointer(
                sourceMgr.getMemoryBuffer(bufId)->getBufferStart() + token.getStart());
            llvm::SMRange range(loc, llvm::SMLoc::getFromPointer(
                sourceMgr.getMemoryBuffer(bufId)->getBufferStart() + token.getEnd()));
            sourceMgr.PrintMessage(loc, llvm::SourceMgr::DK_Error, "invalid token", range);
        }
    }
    if (errorCount != 0) {
        return nullptr;
    }

    lexer = std::make_unique<Lexer>(source);

    yy::parser parser;
    parser.parse();
    lexer.reset();
    assert(bisonProgramResult);
    return bisonProgramResult;
}
