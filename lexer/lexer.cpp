#include "lexer.h"
#include <cctype>
#include <cassert>
#include <algorithm>
#include <memory>


Token Lexer::nextToken() {
    if (dedentCount > 0) {
        dedentCount--;
        return Token(Token::KindDedent, getOffset(index), getOffset(index));
    }

    // skip space
    for (; (*index == '\t' || *index == ' '); index++) {
    }

    if (*index == '\0') {
        return Token(Token::KindEOF, getOffset(index), getOffset(index));
    } else if (auto token = lexNewline(); token.has_value()) {

        if ((int)token.value().getKind() > (int)Token::KindDedent) {
            dedentCount = (int)token.value().getKind() - (int)Token::KindDedent;
            index = strRef.begin() + token.value().getEnd();
            return Token(Token::KindNewline, token.value().getStart(), token.value().getEnd());
        }

        index = strRef.begin() + token.value().getEnd();
        return token.value();
    } else if (auto token = lexFloating(); token.has_value()) {
        index = strRef.begin() + token.value().getEnd();
        return token.value();
    } else if (auto token = lexInteger(); token.has_value()) {
        index = strRef.begin() + token.value().getEnd();
        return token.value();
    } else if (auto token = lexSymbol(); token.has_value()) {
        index = strRef.begin() + token.value().getEnd();
        return token.value();
    } else if (auto token = lexKeyword(); token.has_value()) {
        index = strRef.begin() + token.value().getEnd();
        return token.value();
    } else if (auto token = lexIdent(); token.has_value()) {
        index = strRef.begin() + token.value().getEnd();
        return token.value();
    } else {

        
        // invalid token
        assert(false);
    }
}

//    std::unique_ptr<llvm::MemoryBuffer> buffer = llvm::MemoryBuffer::getMemBufferCopy(strRef, "buffer");
//    assert(buffer);
//
//    llvm::SourceMgr sourceMgr;
//    int bufId = sourceMgr.AddNewSourceBuffer(std::move(buffer), llvm::SMLoc());
//
//
//    indentStack = {""};
//    index = sourceMgr.getMemoryBuffer(bufId)->getBuffer().begin();
//    begin = index;
//    llvm::StringRef::iterator prev = index;
//    std::vector<Token> tokens;

//        } else {
//            size_t distance = std::distance(prev, index);
//            llvm::SMLoc loc = llvm::SMLoc::getFromPointer(
//                    sourceMgr.getMemoryBuffer(bufId)->getBufferStart() + distance);
//
//            llvm::Twine msg = "invalid token";
//            sourceMgr.PrintMessage(loc, llvm::SourceMgr::DK_Error, msg);
//            assert(false);
//        }

std::optional<Token> const Lexer::lexNewline() {
    llvm::StringRef::iterator it = index;
    if (*it != '\n') {
        return std::nullopt;
    }
    it++;
    llvm::StringRef::iterator spacesStart = it;

    for (; (*it == '\t' || *it == ' '); it++) {
    }

    std::string spaces = std::string(spacesStart, it);

    if (spaces == indentStack.back()) {
        return Token(Token::KindNewline, getOffset(index), getOffset(it));
    } else if (spaces.substr(0, indentStack.back().size()) == indentStack.back()) {
        indentStack.push_back(spaces);
        return Token(Token::KindIndent, getOffset(index), getOffset(it));
    } else {
        int level = 0;
        for (; spaces != indentStack.back(); indentStack.pop_back()) {
            assert(indentStack.size() > 0);
            level++;
        }

        assert(spaces == indentStack.back());
        Token::TokenKind kind = (Token::TokenKind)(level + (int)Token::KindDedent);
        return Token(kind, getOffset(index), getOffset(it));
    }
}

std::optional<Token> const Lexer::lexInteger() {
    llvm::StringRef::iterator it = index;
    for (; isdigit(*it); it++) {
    }
    if (it != index) {
        return Token(Token::KindInt, getOffset(index), getOffset(it));
    }
    return std::nullopt;
}


std::optional<Token> const Lexer::lexFloating() {
    llvm::StringRef::iterator it = index;

    if (!isdigit(*it)) {
        return std::nullopt;
    }
    for (; isdigit(*it); it++) {
    }
    if (*it++ != '.') {
        return std::nullopt;
    }
    for (; isdigit(*it); it++) {
    }
    return Token(Token::KindFloat, getOffset(index), getOffset(it));
}

std::optional<Token> const Lexer::lexIdent() {
    llvm::StringRef::iterator it = index;
    if (!isalpha(*it)) {
        return std::nullopt;
    }
    for (; isalpha(*it) || isdigit(*it); it++) {
    }
    return Token(Token::KindIdent, getOffset(index), getOffset(it));
}

std::optional<Token> const Lexer::lexKeyword() {
    llvm::StringRef::iterator it = index;
    if (!isalpha(*it)) {
        return std::nullopt;
    }
    for (; isalpha(*it) || isdigit(*it) || *it == '_' ; it++) {
    }

    auto str = std::string(index, it);
    for (const std::string &keyword : keywords) {
        if (str == keyword) {

            std::cerr << "returning: " << str << '\n';

            return Token(Token::KindKeyword, getOffset(index), getOffset(it));
        }
    }
    return std::nullopt;
}

std::optional<Token> const Lexer::lexSymbol() {
    if (std::find(doubleSymbols.begin(), doubleSymbols.end(), std::string(index, index + 2)) != doubleSymbols.end()) {
        return Token(Token::KindSymbol, getOffset(index), getOffset(index + 2));
    }
    if (symbols.find(*index) != std::string::npos) {
        return Token(Token::KindSymbol, getOffset(index), getOffset(index + 1));
    }
    return std::nullopt;
}
