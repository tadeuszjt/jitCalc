#pragma once

#include <iostream>
#include <string>
#include <optional>
#include <variant>
#include <vector>
// Implements a custom lexer to turn strings into vectors of tokens.


const std::vector keywords = {"fn", "if", "else", "return"};

const std::string symbols = "+-*/()><";
const std::vector<std::string> doubleSymbols = {"=="};

enum TokenSpace {
    SPACE_NEWLINE,
    SPACE_INDENT,
    SPACE_DEDENT
};

struct TokenKeyword {
    TokenKeyword(const std::string &str) : str(str) {}
    const std::string str;
};

struct TokenIdent {
    TokenIdent(const std::string &str) : str(str) {}
    const std::string str;
};

using TokenInt = int;
using TokenFloat = double;
using TokenSymbol = std::string;

using Token = std::variant<TokenInt, TokenFloat, TokenIdent, TokenKeyword, TokenSymbol, TokenSpace>;

std::vector<Token> lexTokens(std::string& str);
std::ostream& operator<<(std::ostream& os, const Token& token);
