#pragma once

#include "ast.h"
#include <string>
#include <memory>
#include <llvm/Support/MemoryBuffer.h>

// provides a simple interface for the bison parser and custom lexer.
ast::Program* parse(llvm::MemoryBuffer &);
