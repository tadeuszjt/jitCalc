#include "parse.h"
#include "lexer.h"
#include "grammar.tab.hh"

#include <cassert>
#include <memory>
#include <sstream>

#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>

static std::unique_ptr<Lexer2> lexer2;
static std::stringstream      stream;
extern ast::Node              *bisonProgramResult;

int yylex(yy::parser::semantic_type *un, yy::parser::location_type *yyloc) {
    auto token = lexer2->nextToken(stream);

    switch (token.type) {
    case Lexer2::Token2::Eof:
        return yy::parser::token::YYEOF;
    case Lexer2::Token2::Integer:
        *un =  new ast::Integer(std::stoi(token.str));
        return yy::parser::token::INTEGER;
    case Lexer2::Token2::Ident:
        *un = new ast::Ident(token.str);
        return yy::parser::token::ident;
    case Lexer2::Token2::Keyword:
        if (token.str == "fn") {
            return yy::parser::token::fn;
        } else if (token.str == "return") {
            return yy::parser::token::Return;
        }
        assert(false);
        break;
    case Lexer2::Token2::Newline:
        return yy::parser::token::NEWLINE;
    case Lexer2::Token2::Indent:
        return yy::parser::token::INDENT;
    case Lexer2::Token2::Dedent:
        return yy::parser::token::DEDENT;
    case Lexer2::Token2::Symbol:
        if (token.str == "+") {
            return '+';
        } else if (token.str == "-") {
            return '-';
        } else if (token.str == "*") {
            return '*';
        } else if (token.str == "/") {
            return '/';
        } else if (token.str == "(") {
            return '(';
        } else if (token.str == ")") {
            return ')';
        } else if (token.str == "==") {
            return yy::parser::token::EqEq;
        }
        assert(false);
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
//    lexer = std::make_unique<Lexer>(source);
    stream = std::stringstream(text);
    lexer2 = std::make_unique<Lexer2>();

//    std::unique_ptr<llvm::MemoryBuffer> buffer = llvm::MemoryBuffer::getMemBufferCopy(source, "buffer");
//    assert(buffer);
//    llvm::SourceMgr sourceMgr;
//    int bufId = sourceMgr.AddNewSourceBuffer(std::move(buffer), llvm::SMLoc());

//    int errorCount = 0;
//    for (;;) {
//        auto token = lexer->nextToken();
//
//
//        if (token.getKind() == Token::KindEOF) {
//            break;
//        }
//        if (token.getKind() == Token::KindInvalid) {
//            errorCount++;
////            llvm::SMLoc loc = llvm::SMLoc::getFromPointer(
////                sourceMgr.getMemoryBuffer(bufId)->getBufferStart() + token.getStart());
////            llvm::SMRange range(loc, llvm::SMLoc::getFromPointer(
////                sourceMgr.getMemoryBuffer(bufId)->getBufferStart() + token.getEnd()));
////            sourceMgr.PrintMessage(loc, llvm::SourceMgr::DK_Error, "invalid token", range);
//        }
//    }
//    if (errorCount != 0) {
//        return nullptr;
//    }

//    lexer = std::make_unique<Lexer>(source);

    yy::parser parser;
    parser.parse();
    lexer2.reset();
    return bisonProgramResult;
}
