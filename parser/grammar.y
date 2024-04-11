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
#include <vector>


// this function is called for every token, returns token type (INTEGER, '+', '-'...)
int yylex(yy::parser::semantic_type *);

// set the result here, free after use
ast::Program *bisonProgramResult = nullptr;


%}

// union defines the yy::parser::semantic_type
%union {
    int integer;
    double floating;
    std::string *identType;
    ast::Stmt *stmtType;
    ast::Expr *exprType;
    std::vector<std::string> *argListType;
}

// INTEGER token type uses the 'integer' field from the union
%token <integer>  INTEGER
%token <identType> ident
%token <floating> FLOATING
%token NEWLINE INDENT DEDENT
%token KW_FN

%token '(' ')' ','
%token '+' '-' '*' '/'

// precedence rules
%left '+' '-'
%left '*'
%left '/'

// expr rule uses the 'expr' field from the union
%type <exprType> expr
%type <stmtType> statement
%type <argListType> argList

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
    | ident '(' ')' { $$ = new ast::Expr(ast::Call(*$1)); delete $1; }
    | '(' expr ')'  { $$ = $2; }
    | '-' expr      { $$ = new ast::Expr(ast::Prefix('-', *$2)); delete $2; }
    | expr '+' expr { $$ = new ast::Expr(ast::Infix(*$1, '+', *$3)); delete $1; delete $3; }
    | expr '-' expr { $$ = new ast::Expr(ast::Infix(*$1, '-', *$3)); delete $1; delete $3; }
    | expr '*' expr { $$ = new ast::Expr(ast::Infix(*$1, '*', *$3)); delete $1; delete $3; }
    | expr '/' expr { $$ = new ast::Expr(ast::Infix(*$1, '/', *$3)); delete $1; delete $3; };

statement
    : KW_FN ident '(' argList ')' INDENT expr DEDENT { $$ = new ast::Stmt(ast::FnDef(*$2, *$4, *$7)); delete $2; delete $4; delete $7; };


argList
    : ident             { $$ = new std::vector<std::string>{*$1}; delete $1; }
    | ident ',' argList { $$ = $3; $$->insert($$->begin(), *$1); delete $1; };

%%

namespace yy {
// provide a definition for the virtual error member
void parser::error(const std::string& msg) {
    std::cerr << "Bison error: " << msg << std::endl;
}
}

