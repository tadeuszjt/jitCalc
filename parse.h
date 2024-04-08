#pragma once

#include "ast.h"
#include <string>

// provides a simple interface for the bison parser and custom lexer.
ast::Expression parse(std::string& text);
