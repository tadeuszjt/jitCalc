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
    | ident '(' exprs ')' { $$ = make_shared<Call>(cast<Ident>($1).get()->ident, cast<List<Node>>($3)); }
    | '-' expr            { $$ = make_shared<Prefix>(Minus, ($2)); }
    | expr '+' expr       { $$ = make_shared<Infix>($1, Plus, $3); }
    | expr '-' expr       { $$ = make_shared<Infix>($1, Minus, $3); }
    | expr '*' expr       { $$ = make_shared<Infix>($1, Times, $3); }
    | expr '/' expr       { $$ = make_shared<Infix>($1, Divide, $3); }
    | expr '<' expr       { $$ = make_shared<Infix>($1, LT, $3); }
    | expr '>' expr       { $$ = make_shared<Infix>($1, GT, $3); }
    | expr EqEq expr      { $$ = make_shared<Infix>($1, EqEq, $3); };


line
    : Return expr { $$ = make_shared<Return>(($2)); };

block
    : fn ident '(' idents ')' INDENT stmts1 DEDENT { $$ = make_shared<FnDef>(cast<Ident>($2), cast<List<Ident>>($4), cast<List<Node>>($7)); }
    | If expr INDENT stmts1 DEDENT                 { $$ = make_shared<If>(($2), cast<List<Node>>($4)); };

stmts1
    : line NEWLINE        { $$ = make_shared<List<Node>>($1); }
    | block               { $$ = make_shared<List<Node>>($1); }
    | line NEWLINE stmts1 { cast<List<Node>>($3)->cons($1); $$ = $3; }
    | block        stmts1 { cast<List<Node>>($2)->cons($1); $$ = $2; };


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
    : expr            { $$ = make_shared<List<Node>>($1); }
    | expr ',' exprs1 { cast<List<Node>>($3)->cons($1); $$ = $3; };
    
%%

namespace yy {
// provide a definition for the virtual error member
void parser::error(const string& msg) {
    cerr << "Bison error: " << msg << endl;
}
}

