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

template <typename A, typename B>
std::shared_ptr<A> cast(std::shared_ptr<B> ptr) { return std::dynamic_pointer_cast<A>(ptr); }

using namespace std;
using namespace ast;
%}


%token NEWLINE INDENT DEDENT INTEGER FLOATING ident
%token KW_FN
%token '(' ')' ','
%token '+' '-' '*' '/'

// precedence rules
%left '+' '-'
%left '*'
%left '/'

%%

program : expr NEWLINE { bisonProgramResult = $1; }
        | stmtBlock    { bisonProgramResult = $1; };

expr
    : INTEGER       { $$ = $1; }
    | FLOATING      { $$ = $1; }
    | '(' expr ')'  { $$ = $2; }
    | ident '(' ')' { $$ = make_shared<Call>(cast<Ident>($1).get()->ident); }
    | '-' expr      { $$ = make_shared<Prefix>('-', cast<Expr>($2)); }
    | expr '+' expr { $$ = make_shared<Infix>(cast<Expr>($1), '+', cast<Expr>($3)); }
    | expr '-' expr { $$ = make_shared<Infix>(cast<Expr>($1), '-', cast<Expr>($3)); }
    | expr '*' expr { $$ = make_shared<Infix>(cast<Expr>($1), '*', cast<Expr>($3)); }
    | expr '/' expr { $$ = make_shared<Infix>(cast<Expr>($1), '/', cast<Expr>($3)); };


stmtBlock
    : KW_FN ident '(' idents ')' INDENT expr DEDENT { $$ = make_shared<FnDef>(cast<Ident>($2), cast<IdentList>($3), cast<Expr>($7)); };



idents : idents1 { $$ = $1; }
       |         { $$ = make_shared<IdentList>(); };
idents1
    : ident             { auto list = make_shared<IdentList>(); list->cons(cast<Ident>($1)); $$ = list; }
    | ident ',' idents1 { cast<IdentList>($3)->cons(cast<Ident>($1)); };
%%

namespace yy {
// provide a definition for the virtual error member
void parser::error(const string& msg) {
    cerr << "Bison error: " << msg << endl;
}
}

