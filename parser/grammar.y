%require "3.2"
%define parse.error verbose

%{
#include "ast.h"
#include "grammar.tab.hh"
#include <string>
#include <iostream>
#include <cassert>
#include <memory>


// this function is called for every token, returns token type (INTEGER, '+', '-'...)
int yylex(yy::parser::semantic_type *);

// set the result here, free after use
ast::Expr *bisonParseResult = nullptr;


%}

// union defines the yy::parser::semantic_type
%union {
    int integer;
    ast::Expr *expr;
    std::string *ident;
}

// INTEGER token type uses the 'integer' field from the union
%token <integer> INTEGER
%token <ident>   IDENT

%token '(' ')'
%token '+' '-' '*' '/'

// precedence rules
%left '+' '-'
%left '*'
%left '/'

// expression rule uses the 'expr' field from the union
%type <expr> expression

%%

program: expression { assert(bisonParseResult == nullptr); bisonParseResult = $1; };

expression
    : INTEGER                   { $$ = new ast::Expr(ast::Integer{$1}); }
    | IDENT '(' ')'             { $$ = new ast::Expr(ast::Call(*$1)); delete $1; }
    | '(' expression ')'        { $$ = $2; }
    | '-' expression            { $$ = new ast::Expr(ast::Prefix('-', *$2)); delete $2; }
    | expression '+' expression { $$ = new ast::Expr(ast::Infix(*$1, '+', *$3)); delete $1; delete $3; }
    | expression '-' expression { $$ = new ast::Expr(ast::Infix(*$1, '-', *$3)); delete $1; delete $3; }
    | expression '*' expression { $$ = new ast::Expr(ast::Infix(*$1, '*', *$3)); delete $1; delete $3; }
    | expression '/' expression { $$ = new ast::Expr(ast::Infix(*$1, '/', *$3)); delete $1; delete $3; };

%%

namespace yy {
// provide a definition for the virtual error member
void parser::error(const std::string& msg) {
    std::cerr << "Bison error: " << msg << std::endl;
}
}

