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
std::shared_ptr<A> cast(std::shared_ptr<B> ptr) {
    auto casted = std::dynamic_pointer_cast<A>(ptr);
    assert(casted.get() != nullptr);
    return casted;
}

using namespace std;
using namespace ast;
%}


%token NEWLINE INDENT DEDENT INTEGER FLOATING ident
%token fn If Else Return
%token '(' ')' ','
%token '+' '-' '*' '/' '<' '>' EqEq

// precedence rules
%left EqEq
%left '>' '<'
%left '+' '-'
%left '*' '/'

%%

program : expr NEWLINE { bisonProgramResult = $1; }
        | block        { bisonProgramResult = $1; };

expr
    : INTEGER             { $$ = $1; }
    | FLOATING            { $$ = $1; }
    | '(' expr ')'        { $$ = $2; }
    | ident               { $$ = $1; }
    | ident '(' exprs ')' { $$ = make_shared<Call>(cast<Ident>($1).get()->ident, cast<List<Expr>>($3)); }
    | '-' expr            { $$ = make_shared<Prefix>(Minus, cast<Expr>($2)); }
    | expr '+' expr       { $$ = make_shared<Infix>(cast<Expr>($1), Plus, cast<Expr>($3)); }
    | expr '-' expr       { $$ = make_shared<Infix>(cast<Expr>($1), Minus, cast<Expr>($3)); }
    | expr '*' expr       { $$ = make_shared<Infix>(cast<Expr>($1), Times, cast<Expr>($3)); }
    | expr '/' expr       { $$ = make_shared<Infix>(cast<Expr>($1), Divide, cast<Expr>($3)); }
    | expr '<' expr       { $$ = make_shared<Infix>(cast<Expr>($1), LT, cast<Expr>($3)); }
    | expr '>' expr       { $$ = make_shared<Infix>(cast<Expr>($1), GT, cast<Expr>($3)); }
    | expr EqEq expr      { $$ = make_shared<Infix>(cast<Expr>($1), EqEq, cast<Expr>($3)); };


line
    : Return expr { $$ = make_shared<Return>(cast<Expr>($2)); };

block
    : fn ident '(' idents ')' INDENT stmts1 DEDENT { $$ = make_shared<FnDef>(cast<Ident>($2), cast<List<Ident>>($4), cast<List<Stmt>>($7)); }
    | If expr INDENT stmts1 DEDENT                 { $$ = make_shared<If>(cast<Expr>($2), cast<List<Stmt>>($4)); };

stmts1
    : line NEWLINE        { $$ = make_shared<List<Stmt>>(cast<Stmt>($1)); }
    | block               { $$ = make_shared<List<Stmt>>(cast<Stmt>($1)); }
    | line NEWLINE stmts1 { cast<List<Stmt>>($3)->cons(cast<Stmt>($1)); $$ = $3; }
    | block        stmts1 { cast<List<Stmt>>($2)->cons(cast<Stmt>($1)); $$ = $2; };


idents
    : idents1 { $$ = $1; }
    |         { $$ = make_shared<List<Ident>>(); };
idents1
    : ident             { $$ = make_shared<List<Ident>>(cast<Ident>($1)); }
    | ident ',' idents1 { cast<List<Ident>>($3)->cons(cast<Ident>($1)); $$ = $3; };


exprs
    : exprs1 { $$ = $1; }
    |        { $$ = make_shared<List<Ident>>(); };
exprs1
    : expr            { $$ = make_shared<List<Expr>>(cast<Expr>($1)); }
    | expr ',' exprs1 { cast<List<Expr>>($3)->cons(cast<Expr>($1)); $$ = $3; };
    
%%

namespace yy {
// provide a definition for the virtual error member
void parser::error(const string& msg) {
    cerr << "Bison error: " << msg << endl;
}
}

