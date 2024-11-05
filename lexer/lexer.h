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

std::ostream& operator<<(std::ostream& os, const Token& token);



class Lexer {
public:
    std::vector<Token> lexTokens(llvm::StringRef str);

private:
    std::vector<TokenSpace> lexNewline();
    std::optional<Token> lexInteger();
    std::optional<Token> lexFloating();
    std::optional<Token> lexIdent();
    std::optional<Token> lexKeyword();
    std::optional<Token> lexSymbol();

    llvm::StringRef::iterator start;
    std::vector<std::string> indentStack;
};
