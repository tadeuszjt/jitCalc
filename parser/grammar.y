%require "3.2"
%define parse.error verbose

%{
#include "ast.h"
#include "grammar.tab.hh"
#include <string>
#include <iostream>
#include <cassert>


// this function is called for every token, returns token type (INTEGER, '+', '-'...)
int yylex(yy::parser::semantic_type *);

// set the result here, free after use
ast::Expression *bisonParseResult = nullptr;


%}

// union defines the yy::parser::semantic_type
%union {
    int integer;
    ast::Expression *expr;
}

// INTEGER token type uses the 'integer' field from the union
%token <integer> INTEGER

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

expression: INTEGER { $$ = new ast::Expression(ast::Integer{$1}); }
          | '(' expression ')' { $$ = $2; }
//          | '-' expression {
//    $$ = new ast::Expression(ast::Prefix('-', *$2));
//    delete $2;
//}
          | expression '+' expression {
    $$ = new ast::Expression(ast::Infix(*$1, '+', *$3));
    delete $1;
    delete $3;
}
          | expression '-' expression {
    $$ = new ast::Expression(ast::Infix(*$1, '-', *$3));
    delete $1;
    delete $3;
}
          | expression '*' expression {
    $$ = new ast::Expression(ast::Infix(*$1, '*', *$3));
    delete $1;
    delete $3;
}
          | expression '/' expression {
    $$ = new ast::Expression(ast::Infix(*$1, '/', *$3));
    delete $1;
    delete $3;
};

%%

namespace yy {
// provide a definition for the virtual error member
void parser::error(const std::string& msg) {
    std::cerr << "Bison error: " << msg << std::endl;
}
}

