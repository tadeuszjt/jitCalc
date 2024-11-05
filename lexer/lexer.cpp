#include "lexer.h"
#include <cctype>
#include <cassert>
#include <algorithm>
#include <memory>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>

std::vector<Token> Lexer::lexTokens(llvm::StringRef str) {
    std::unique_ptr<llvm::MemoryBuffer> buffer = llvm::MemoryBuffer::getMemBufferCopy(str, "buffer");
    assert(buffer);

    llvm::SourceMgr sourceMgr;
    int bufId = sourceMgr.AddNewSourceBuffer(std::move(buffer), llvm::SMLoc());


    indentStack = {""};
    index = sourceMgr.getMemoryBuffer(bufId)->getBuffer().begin();
    begin = index;
    llvm::StringRef::iterator prev = index;
    std::vector<Token> tokens;

    for (;;) {
        // skip space
        for (; (*index == '\t' || *index == ' '); index++) {
        }

        if (*index == '\0') {
            break;
        } else if (auto indent = lexNewline(); indent.size() > 0) {
            for (auto nl : indent) {
                tokens.push_back(nl);
            }
        } else if (auto token = lexFloating(); token.has_value()) {
            tokens.push_back(token.value());
        } else if (auto token = lexInteger(); token.has_value()) {
            tokens.push_back(token.value());
        } else if (auto token = lexSymbol(); token.has_value()) {
            tokens.push_back(token.value());
        } else if (auto token = lexKeyword(); token.has_value()) {
            tokens.push_back(token.value());
        } else if (auto token = lexIdent(); token.has_value()) {
            tokens.push_back(token.value());
        } else {
            size_t distance = std::distance(prev, index);
            llvm::SMLoc loc = llvm::SMLoc::getFromPointer(
                    sourceMgr.getMemoryBuffer(bufId)->getBufferStart() + distance);

            llvm::Twine msg = "invalid token";
            sourceMgr.PrintMessage(loc, llvm::SourceMgr::DK_Error, msg);
            assert(false);
        }
    }

    return tokens;
}

std::vector<Token> Lexer::lexNewline() {
    llvm::StringRef::iterator prev = index;
    if (*index != '\n') {
        return {};
    }
    index++;
    llvm::StringRef::iterator spacesStart = index;

    for (; (*index == '\t' || *index == ' '); index++) {
    }

    std::string spaces = std::string(spacesStart, index);

    if (spaces == indentStack.back()) {
        return {Token(Token::KindNewline, getOffset(prev), getOffset(index))};
    } else if (spaces.substr(0, indentStack.back().size()) == indentStack.back()) {
        indentStack.push_back(spaces);
        return {Token(Token::KindIndent, getOffset(prev), getOffset(index))};
    } else {
        std::vector<Token> tokens = {Token(Token::KindNewline, getOffset(prev), getOffset(index))};
        for (; spaces != indentStack.back(); indentStack.pop_back()) {
            assert(indentStack.size() > 0);
            tokens.push_back(Token(Token::KindDedent, getOffset(prev), getOffset(index)));
        }

        assert(spaces == indentStack.back());
        return tokens;
    }
}

std::optional<Token> Lexer::lexInteger() {
    llvm::StringRef::iterator prev = index;
    for (; isdigit(*index); index++) {
    }
    if (prev != index) {
        return Token(Token::KindInt, getOffset(prev), getOffset(index));
    }
    return std::nullopt;
}


std::optional<Token> Lexer::lexFloating() {
    llvm::StringRef::iterator it = index;

    for (; isdigit(*it); it++) {
    }
    if (it == index) {
        return std::nullopt;
    }
    if (*it++ != '.') {
        return std::nullopt;
    }
    for (; isdigit(*it); it++) {
    }
    llvm::StringRef::iterator start = index;
    index = it;
    return Token(Token::KindFloat, getOffset(start), getOffset(index));
}

std::optional<Token> Lexer::lexIdent() {
    llvm::StringRef::iterator prev = index;
    if (!isalpha(*index)) {
        return std::nullopt;
    }
    for (; isalpha(*index) || isdigit(*index); index++) {
    }
    return Token(Token::KindIdent, getOffset(prev), getOffset(index));
}

std::optional<Token> Lexer::lexKeyword() {
    llvm::StringRef::iterator prev = index;
    llvm::StringRef::iterator it = index;
    if (!isalpha(*it)) {
        return std::nullopt;
    }
    for (; isalpha(*it) || isdigit(*it) || *it == '_' ; it++) {
    }

    auto str = std::string(index, it);
    for (const std::string &keyword : keywords) {
        if (str == keyword) {
            index = it;
            return Token(Token::KindKeyword, getOffset(prev), getOffset(it));
        }
    }
    return std::nullopt;
}

std::optional<Token> Lexer::lexSymbol() {
    llvm::StringRef::iterator prev = index;

    if (std::find(doubleSymbols.begin(), doubleSymbols.end(), std::string(index, index + 2)) != doubleSymbols.end()) {
        index += 2;
        return Token(Token::KindSymbol, getOffset(prev), getOffset(index));
    }
    if (symbols.find(*index) != std::string::npos) {
        index++;
        return Token(Token::KindSymbol, getOffset(prev), getOffset(index));
    }

    return std::nullopt;
}
