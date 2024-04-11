%require "3.2"
%define parse.error verbose
%define api.value.type { std::shared_ptr<ast::Node> }

%{
#include "ast.h"
#include "grammar.tab.hh"
#include <string>
#include <iostream>
#include <cassert>
#include <memory>
#include <optional>
#include <vector>


// this function is called for every token, returns token type (INTEGER, '+', '-'...)
int yylex(yy::parser::semantic_type *);
std::shared_ptr<ast::Node> bisonProgramResult;
%}


%token NEWLINE INDENT DEDENT INTEGER FLOATING ident
%token '(' ')' ','
%token '+' '-' '*' '/'

// precedence rules
%left '+' '-'
%left '*'
%left '/'

%%

program : expr NEWLINE { bisonProgramResult = $1; }

expr : INTEGER       { $$ = $1; }
     | FLOATING      { $$ = $1; }
     | ident '(' ')' { $$ = std::make_shared<ast::Call>(std::dynamic_pointer_cast<ast::Ident>($1).get()->ident); }
     | '(' expr ')'  { $$ = $2; }
     | '-' expr      { $$ = std::make_shared<ast::Prefix>('-', std::dynamic_pointer_cast<ast::Expr>($2)); }
     | expr '+' expr { $$ = std::make_shared<ast::Infix>(std::dynamic_pointer_cast<ast::Expr>($1), '+', std::dynamic_pointer_cast<ast::Expr>($3)); }
     | expr '-' expr { $$ = std::make_shared<ast::Infix>(std::dynamic_pointer_cast<ast::Expr>($1), '-', std::dynamic_pointer_cast<ast::Expr>($3)); }
     | expr '*' expr { $$ = std::make_shared<ast::Infix>(std::dynamic_pointer_cast<ast::Expr>($1), '*', std::dynamic_pointer_cast<ast::Expr>($3)); }
     | expr '/' expr { $$ = std::make_shared<ast::Infix>(std::dynamic_pointer_cast<ast::Expr>($1), '/', std::dynamic_pointer_cast<ast::Expr>($3)); };

//program: expr NEWLINE {
//       assert(bisonProgramResult == nullptr);
//       bisonProgramResult = new ast::Program(*$1);
//       delete $1;
//}
//    | statement {
//       assert(bisonProgramResult == nullptr);
//       bisonProgramResult = new ast::Program(*$1);
//       delete $1;
//};
//
//
//statement
//    : KW_FN ident '(' argList ')' INDENT expr DEDENT { $$ = new ast::Stmt(ast::FnDef(*$2, *$4, *$7)); delete $2; delete $4; delete $7; };
//
//
//argList
//    : ident             { $$ = new std::vector<std::string>{*$1}; delete $1; }
//    | ident ',' argList { $$ = $3; $$->insert($$->begin(), *$1); delete $1; };

%%

namespace yy {
// provide a definition for the virtual error member
void parser::error(const std::string& msg) {
    std::cerr << "Bison error: " << msg << std::endl;
}
}

