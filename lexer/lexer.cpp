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
    start = sourceMgr.getMemoryBuffer(bufId)->getBuffer().begin();
    llvm::StringRef::iterator prev = start;
    std::vector<Token> tokens;

    for (;;) {
        // skip space
        for (; (*start == '\t' || *start == ' '); start++) {
        }

        if (*start == '\0') {
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
            size_t distance = std::distance(prev, start);
            llvm::SMLoc loc = llvm::SMLoc::getFromPointer(
                    sourceMgr.getMemoryBuffer(bufId)->getBufferStart() + distance);

            llvm::Twine msg = "invalid token";
            sourceMgr.PrintMessage(loc, llvm::SourceMgr::DK_Error, msg);
            assert(false);
        }
    }

    return tokens;
}

std::vector<TokenSpace> Lexer::lexNewline() {
    if (*start != '\n') {
        return {};
    }
    start++;
    llvm::StringRef::iterator spacesStart = start;

    for (; (*start == '\t' || *start == ' '); start++) {
    }

    std::string spaces = std::string(spacesStart, start);

    if (spaces == indentStack.back()) {
        return {SPACE_NEWLINE};
    } else if (spaces.substr(0, indentStack.back().size()) == indentStack.back()) {
        indentStack.push_back(spaces);
        return {SPACE_INDENT};
    } else {
        std::vector<TokenSpace> tokens = {SPACE_NEWLINE};
        for (; spaces != indentStack.back(); indentStack.pop_back()) {
            assert(indentStack.size() > 0);
            tokens.push_back(SPACE_DEDENT);
        }

        assert(spaces == indentStack.back());
        return tokens;
    }
}

std::optional<Token> Lexer::lexInteger() {
    llvm::StringRef::iterator prev = start;
    for (; isdigit(*start); start++) {
    }
    if (prev != start) {
        return TokenInt{std::stoi(std::string(prev, start))};
    }
    return std::nullopt;
}


std::optional<Token> Lexer::lexFloating() {
    llvm::StringRef::iterator it = start;
    llvm::StringRef::iterator prev = start;

    for (; isdigit(*it); it++) {
    }
    if (it == start) {
        return std::nullopt;
    }
    if (*it++ != '.') {
        return std::nullopt;
    }
    for (; isdigit(*it); it++) {
    }
    start = it;
    return TokenFloat{std::stod(std::string(prev, start))};
}

std::optional<Token> Lexer::lexIdent() {
    llvm::StringRef::iterator prev = start;
    if (!isalpha(*start)) {
        return std::nullopt;
    }
    for (; isalpha(*start) || isdigit(*start); start++) {
    }
    return TokenIdent{std::string(prev, start)};
}

std::optional<Token> Lexer::lexKeyword() {
    llvm::StringRef::iterator it = start;
    if (!isalpha(*it)) {
        return std::nullopt;
    }
    for (; isalpha(*it) || isdigit(*it) || *it == '_' ; it++) {
    }

    auto str = std::string(start, it);
    for (const std::string &keyword : keywords) {
        if (str == keyword) {
            start = it;
            return TokenKeyword(str);
        }
    }
    return std::nullopt;
}

std::optional<Token> Lexer::lexSymbol() {
    llvm::StringRef::iterator prev = start;

    if (std::find(doubleSymbols.begin(), doubleSymbols.end(), std::string(start, start + 2)) != doubleSymbols.end()) {
        start += 2;
        return TokenSymbol{std::string(prev, start)};
    }
    if (symbols.find(*start) != std::string::npos) {
        start++;
        return TokenSymbol{std::string(prev, start)};
    }

    return std::nullopt;
}


std::ostream& operator<<(std::ostream& os, const Token& token) {
    if (std::holds_alternative<TokenInt>(token)) {
        os << "TokenInt(" << std::get<TokenInt>(token) << ")";
    } else if (std::holds_alternative<TokenIdent>(token)) {
        os << "TokenIdent(" << std::get<TokenIdent>(token).str << ")";
    } else if (std::holds_alternative<TokenKeyword>(token)) {
        os << "TokenKeyword(" << std::get<TokenKeyword>(token).str << ")";
    } else if (std::holds_alternative<TokenSymbol>(token)) {
        os << "TokenSymbol(" << std::get<TokenSymbol>(token) << ")";
    } else if (std::holds_alternative<TokenSpace>(token)) {
        switch (std::get<TokenSpace>(token)) {
        case SPACE_NEWLINE: os << "TokenSpace(NEWLINE)"; break;
        case SPACE_INDENT: os << "TokenSpace(INDENT)"; break;
        case SPACE_DEDENT: os << "TokenSpace(DEDENT)"; break;
        default: assert(false); break;
        }
    } else {
        assert(false);
    }
    return os;
}
