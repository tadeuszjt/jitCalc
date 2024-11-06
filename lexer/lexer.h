#pragma once

#include <iostream>
#include <string>
#include <optional>
#include <variant>
#include <vector>
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
