#pragma once

#include "ast.h"
#include <string>
#include <memory>

// provides a simple interface for the bison parser and custom lexer.
ast::Node* parse(std::string& text);
