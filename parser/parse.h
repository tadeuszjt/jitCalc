#pragma once

#include "ast.h"
#include "sparse.h"
#include <string>
#include <memory>
#include <llvm/Support/MemoryBuffer.h>

// provides a simple interface for the bison parser and custom lexer.
Sparse<ast::Node>* parse(Sparse<ast::Node>::Key &, llvm::MemoryBuffer &);
