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
    Lexer(llvm::StringRef strRef) : strRef(strRef), indentStack({""}), index(strRef.begin()), begin(strRef.begin()) {}
    Token nextToken();

private:
    size_t getOffset(llvm::StringRef::iterator it) { return std::distance(begin, it); }

    std::optional<Token> lexNewline();
    std::optional<Token> lexInteger();
    std::optional<Token> lexFloating();
    std::optional<Token> lexIdent();
    std::optional<Token> lexKeyword();
    std::optional<Token> lexSymbol();

    llvm::StringRef strRef;
    llvm::StringRef::iterator begin;
    llvm::StringRef::iterator index;
    std::vector<std::string> indentStack;
};
