#include "lexer.h"
#include <cctype>
#include <cassert>
#include <algorithm>
#include <memory>

Lexer2::Lexer2() {
    indentStack = {""};
    dedentCount = 0;
}


Lexer2::Token2 Lexer2::nextToken(std::istream &istream) {
    for (;;) {
        auto nextChar = peek(0, istream);
        if (nextChar.c == ' ' || nextChar.c == '\t') {
            get(istream);
        } else {
            break;
        }
    }

    if (dedentCount > 0) {
        dedentCount--;
        Lexer2::Token2 token{.type = Lexer2::Token2::Dedent, .str = "dedent"};
        return token;
    } else if (peek(0, istream).c == EOF) {
        Lexer2::Token2 token{.type = Lexer2::Token2::Eof, .str = "Eof"};
        return token;
    } else if (auto token = lexInteger(istream); token.has_value()) {
        return token.value();
    } else if (auto token = lexIndent(istream); token.has_value()) {
        return token.value();
    } else if (auto token = lexSymbol(istream); token.has_value()) {
        return token.value();
    } else if (auto token = lexKeyword(istream); token.has_value()) {
        return token.value();
    } else if (auto token = lexIdent(istream); token.has_value()) {
        return token.value();
    } else {
        assert(false);
    }
}


std::optional<Lexer2::Token2> Lexer2::lexInteger(std::istream &istream) {
    auto first = peek(0, istream);
    if (!isdigit(first.c)) {
        return std::nullopt;
    }
    Lexer2::Token2 token{.type = Lexer2::Token2::Integer, .begin = first.pos};

    while (isdigit(peek(0, istream).c)) {
        token.str.push_back(get(istream).c);
    }

    token.end = peek(0, istream).pos;
    return token;
}

std::optional<Lexer2::Token2> Lexer2::lexIdent(std::istream &istream) {
    if (!isalpha(peek(0, istream).c)) {
        return std::nullopt;
    }
    Lexer2::Token2 token{.type = Lexer2::Token2::Ident, .begin = peek(0, istream).pos};

    for (;;) {
        auto ch = peek(0, istream);
        if (!(isalpha(ch.c) || isdigit(ch.c))) {
            break;
        } else {
            get(istream);
            token.str.push_back(ch.c);
        }
    }

    token.end = peek(0, istream).pos;
    return token;
}

std::optional<Lexer2::Token2> Lexer2::lexKeyword(std::istream &istream) {
    if (!isalpha(peek(0, istream).c)) {
        return std::nullopt;
    }

    Lexer2::Token2 token{.type = Lexer2::Token2::Keyword, .begin = peek(0, istream).pos};

    for (int i = 0;; i++) {
        auto ch = peek(i, istream);
        if (isalpha(ch.c) || isdigit(ch.c) || ch.c == '_') {
            token.str.push_back(ch.c);
        } else {
            break;
        }
    }

    for (auto & keyword : keywords) {
        if (token.str == keyword) {
            for (int i = 0; i < token.str.size(); i++) {
                get(istream);
            }
            token.end = peek(0, istream).pos;
            return token;
        }
    }

    return std::nullopt;
}

std::optional<Lexer2::Token2> Lexer2::lexSymbol(std::istream &istream) {
    std::string doubleSymbol{peek(0, istream).c, peek(1, istream).c};

    for (auto &str : doubleSymbols) {
        if (str == doubleSymbol) {
            Lexer2::Token2 token{
                .type = Lexer2::Token2::Symbol,
                .begin = peek(0, istream).pos,
                .end = peek(2, istream).pos,
                .str = doubleSymbol
            };

            get(istream);
            get(istream);
            return token;
        }
    }

    std::string symbol{peek(0, istream).c};

    for (auto &str : symbols) {
        if (std::string{str} == symbol) {
            Lexer2::Token2 token{
                .type = Lexer2::Token2::Symbol,
                .begin = peek(0, istream).pos,
                .end = peek(1, istream).pos,
                .str = symbol
            };

            get(istream);
            return token;
        }
    }

    return std::nullopt;
}


std::optional<Lexer2::Token2> Lexer2::lexIndent(std::istream &istream) {
    if (peek(0, istream).c != '\n') {
        return std::nullopt;
    }

    std::string spaces;
    for (;;) {
        auto ch = peek(0, istream);
        if (ch.c == '\n' || ch.c == ' ' || ch.c == '\t') {
            spaces.push_back(ch.c);
            get(istream);
        } else {
            break; 
        }
    }

    std::string indent = spaces.substr(spaces.find_last_of('\n') + 1);
    Lexer2::Token2 token{
        .begin = peek(0, istream).pos,
        .end = peek(spaces.size(), istream).pos,
        .str = indent
    };

    if (indent == indentStack.back()) {
        token.type = Lexer2::Token2::Newline;
        token.str  = "newline";
    } else if (indent.substr(0, indentStack.back().size()) == indentStack.back()) {
        indentStack.push_back(indent);
        token.type = Lexer2::Token2::Indent;
        token.str  = "indent"; 
    } else if (indentStack.back().substr(0, indent.size()) == indent) {
        for (;;) {
            if (indentStack.size() == 0) {
                assert(false);
            } else if (indentStack.back() == indent) {
                break;
            } else {
                indentStack.pop_back();
                dedentCount++;
            }
        }

        printf("here: %d\n", dedentCount);
        token.type = Lexer2::Token2::Newline;
        token.str  = "newline";
    } else {
        assert(false);
    }

    return token;
}


Lexer2::TextChar Lexer2::peek(size_t n, std::istream &istream) {
    while (n >= charQueue.size()) {
        auto ch = nextCharFromStream(istream);
        charQueue.push_front(ch);
    }

    return charQueue[charQueue.size() - 1 - n];
}

Lexer2::TextChar Lexer2::get(std::istream &istream) {
    if (charQueue.size() > 0) {
        auto ch = charQueue.back();
        charQueue.pop_back();
        return ch;
    }
    return nextCharFromStream(istream);
}


Lexer2::TextChar Lexer2::nextCharFromStream(std::istream &istream) {
    char c = istream.get();
    TextPos pos = curPos;

    if (c == EOF) {
        return Lexer2::TextChar(EOF, pos);
    }

    if (c == '\n') {
        curPos.line++; 
        curPos.column = 1;
    } else {
        curPos.column++;
    }
    curPos.index++;

    return Lexer2::TextChar(c, pos);
}
