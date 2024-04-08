#pragma once

#include <iostream>
#include <string>
#include <optional>
#include <variant>
#include <vector>
// Implements a custom lexer to turn strings into vectors of tokens.

using TokenInt = int;
using TokenIdent = std::string;
using TokenSymbol = char;
using Token = std::variant<TokenInt, TokenIdent, TokenSymbol>;

struct LexSuccess {
    std::string::iterator rest;
    Token token;
    LexSuccess(std::string::iterator it, Token tok) : rest(it), token(tok) {};
};

using LexResult = std::optional<LexSuccess>;


std::ostream& operator<<(std::ostream& os, const Token& token);

std::vector<Token> lexTokens(std::string& str);

LexResult lexNumber(std::string::iterator begin, std::string::iterator end);
LexResult lexIdent(std::string::iterator begin, std::string::iterator end);
LexResult lexAny(std::string::iterator begin, std::string::iterator end);
LexResult lexSymbol(std::string::iterator begin, std::string::iterator end);

