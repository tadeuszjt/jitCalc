#pragma once

#include "ast.h"
#include <string>
#include <memory>

// provides a simple interface for the bison parser and custom lexer.
std::shared_ptr<ast::Node> parse(std::string& text);
