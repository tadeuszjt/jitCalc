#pragma once

#include <iostream>
#include <string>
#include <optional>
#include <variant>
#include <vector>
#include <deque>
#include <sstream>
#include <llvm/ADT/StringRef.h>
// Implements a custom lexer to turn strings into vectors of tokens.


const std::vector keywords = {"fn", "if", "else", "return"};
const std::string symbols = "+-*/()><";
const std::vector<std::string> doubleSymbols = {"=="};


class Lexer2 {
    struct TextPos {
        TextPos() : line(0), column(0), index(0) {}
        void operator=(TextPos a) { line = a.line; column = a.column; index = a.index; }
        size_t line;
        size_t column;
        size_t index;
    };

    struct TextChar {
        TextChar(char c, TextPos p) : c(c), pos(p) {}
        char c;
        TextPos pos;
    };


public:
    struct Token2 {
        enum TokenType { Integer, Ident, Keyword, Symbol, Newline, Indent, Dedent, Eof };
        TokenType type;
        TextPos begin;
        TextPos end;
        std::string str;
    };

    Lexer2();
    Token2 nextToken(std::istream &istream);
    std::optional<Token2> lexInteger(std::istream &istream);
    std::optional<Token2> lexIdent(std::istream &istream);
    std::optional<Token2> lexKeyword(std::istream &istream);
    std::optional<Token2> lexSymbol(std::istream &istream);
    std::optional<Token2> lexIndent(std::istream &istream);

private:
    TextPos curPos;
    std::deque<TextChar> charQueue;
    std::vector<std::string> indentStack;
    size_t dedentCount;

    TextChar peek(size_t n, std::istream &istream);
    TextChar get(std::istream &istream);
    TextChar nextCharFromStream(std::istream &istream);
};
