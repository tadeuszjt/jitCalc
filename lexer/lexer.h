#pragma once

#include <iostream>
#include <string>
#include <optional>
#include <variant>
#include <vector>
#include <deque>
#include <llvm/ADT/StringRef.h>
// Implements a custom lexer to turn strings into vectors of tokens.


const std::vector keywords = {"fn", "if", "else", "return"};
const std::string symbols = "+-*/()><";
const std::vector<std::string> doubleSymbols = {"=="};


class Token {
public:
    enum TokenKind {KindInt, KindFloat, KindKeyword, KindIdent, KindSymbol,
        KindIndent, KindNewline, KindEOF, KindInvalid, KindDedent/* must be end*/};

    Token(TokenKind kind, size_t start, size_t end)
        : kind(kind), start(start), end(end) {}
    TokenKind getKind() { return kind; }
    size_t    getStart() { return start; }
    size_t    getEnd() { return end; }
private:
    size_t start, end;
    TokenKind kind;
};


class Lexer {
public:
    Lexer(llvm::StringRef str) : strRef(strRef), indentStack({""}) {
        strRef = str;
        index = strRef.begin();
        dedentCount = 0;
    }
    Token nextToken();

private:
    size_t getOffset(llvm::StringRef::iterator it) const { return std::distance(strRef.begin(), it); }

    std::optional<Token> lexNewline();
    std::optional<Token> lexInteger() const;
    std::optional<Token> lexFloating() const;
    std::optional<Token> lexIdent() const;
    std::optional<Token> lexKeyword() const;
    std::optional<Token> lexSymbol() const;

    int dedentCount;
    llvm::StringRef strRef;
    llvm::StringRef::iterator index;
    std::vector<std::string> indentStack;
};


//class Lexer2 {
//    struct TextPos {
//        TextPos() : line(0), column(0), index(0) {}
//        void operator=(TextPos a) { line = a.line; column = a.column; index = a.index; }
//        size_t line;
//        size_t column;
//        size_t index;
//    };
//
//    struct TextChar {
//        TextChar(char c, TextPos p) : c(c), pos(p) {}
//        char c;
//        TextPos pos;
//    };
//
//
//public:
//    struct Token2 {
//        enum TokenType { TokInteger };
//        TokenType type;
//        TextPos begin;
//        TextPos end;
//        std::string str;
//    };
//
//
//    Lexer2() {
//    }
//
//
//    std::optional<Token2> lexInteger(std::istream &istream) {
//        auto first = peek(1, istream);
//        if (!isdigit(first.c)) {
//            return std::nullopt;
//        }
//
//        std::string digits;
//        for (int i = 1;; i++) {
//            auto next = peek(i);
//
//
//        }
//
//        std::ve
//    }
//
//private:
//    TextPos curPos;
//    std::deque<TextChar> charQueue;
//
//
//    TextChar peek(size_t n, std::istream &istream) {
//        if (n < charQueue.size()) {
//            return charQueue[n];
//        }
//
//        for (int i = charQueue.size(); i < n; i++) {
//            auto ch = nextCharFromStream(istream);
//            if (ch.has_value()) {
//                charQueue.push_front(ch.value());
//            } else {
//                return TextChar(EOF, TextPos());
//            }
//        }
//
//        return charQueue.front();
//    }
//
//    std::optional<TextChar> get(std::istream &istream) {
//        if (charQueue.size() > 0) {
//            auto ch = charQueue.back();
//            charQueue.pop_back();
//            return ch;
//        }
//        return nextCharFromStream(istream);
//    }
//
//
//    std::optional<TextChar> nextCharFromStream(std::istream &istream) {
//        char c = istream.get();
//        TextPos pos = curPos;
//
//        if (c == EOF) {
//            return std::nullopt;
//        }
//
//        if (c == '\n') {
//            curPos.line++; 
//            curPos.column = 1;
//        } else {
//            curPos.column++;
//        }
//        curPos.index++;
//
//        return TextChar(c, pos);
//    }
//};
