%require "3.2"
%define parse.error verbose

%{
#include "ast.h"
#include "grammar.tab.hh"
#include <string>
#include <iostream>
#include <cassert>
#include <memory>
#include <optional>


// this function is called for every token, returns token type (INTEGER, '+', '-'...)
int yylex(yy::parser::semantic_type *);

// set the result here, free after use
ast::Program *bisonProgramResult = nullptr;


%}

// union defines the yy::parser::semantic_type
%union {
    int integer;
    double floating;
    std::string *ident;
    ast::Stmt *stmtType;
    ast::Expr *exprType;
}

// INTEGER token type uses the 'integer' field from the union
%token <integer>  INTEGER
%token <ident>    IDENT
%token <ident>    KEYWORD
%token <floating> FLOATING
%token NEWLINE INDENT DEDENT
%token KW_FN

%token '(' ')'
%token '+' '-' '*' '/'

// precedence rules
%left '+' '-'
%left '*'
%left '/'

// expr rule uses the 'expr' field from the union
%type <exprType> expr
%type <stmtType> statement

%%

program: expr NEWLINE {
       assert(bisonProgramResult == nullptr);
       bisonProgramResult = new ast::Program(*$1);
       delete $1;
}
    | statement {
       assert(bisonProgramResult == nullptr);
       bisonProgramResult = new ast::Program(*$1);
       delete $1;
};


expr
    : INTEGER       { $$ = new ast::Expr(ast::Integer{$1}); }
    | FLOATING      { $$ = new ast::Expr(ast::Floating{$1}); }
    | IDENT '(' ')' { $$ = new ast::Expr(ast::Call(*$1)); delete $1; }
    | '(' expr ')'  { $$ = $2; }
    | '-' expr      { $$ = new ast::Expr(ast::Prefix('-', *$2)); delete $2; }
    | expr '+' expr { $$ = new ast::Expr(ast::Infix(*$1, '+', *$3)); delete $1; delete $3; }
    | expr '-' expr { $$ = new ast::Expr(ast::Infix(*$1, '-', *$3)); delete $1; delete $3; }
    | expr '*' expr { $$ = new ast::Expr(ast::Infix(*$1, '*', *$3)); delete $1; delete $3; }
    | expr '/' expr { $$ = new ast::Expr(ast::Infix(*$1, '/', *$3)); delete $1; delete $3; };

statement
    : KW_FN IDENT '(' ')' INDENT expr DEDENT { $$ = new ast::Stmt(ast::FnDef()); delete $6; };

%%

namespace yy {
// provide a definition for the virtual error member
void parser::error(const std::string& msg) {
    std::cerr << "Bison error: " << msg << std::endl;
}
}

