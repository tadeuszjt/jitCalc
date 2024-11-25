%require "3.2"
%define parse.error verbose
%define api.value.type { ast::Node* }
%locations

%{
#include "ast.h"
#include "lexer.h"
#include "grammar.tab.hh"
#include <string>
#include <iostream>
#include <cassert>
#include <memory>
#include <optional>
#include <vector>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>


// this function is called for every token, returns token type (INTEGER, '+', '-'...)
int yylex(yy::parser::semantic_type *, yy::parser::location_type *);
ast::Program* bisonProgramResult;

template <typename A, typename B>
A* cast(B* ptr) {
    auto *casted = llvm::dyn_cast<A>(ptr);
    assert(casted != nullptr);
    return casted;
}

TextPos textPos(const yy::parser::location_type &loc) {
    return TextPos(loc.begin.line, loc.begin.column, 0);
}

using namespace std;
using namespace ast;
%}


%token NEWLINE INDENT DEDENT INTEGER FLOATING ident
%token fn If Else Return Let For
%token '(' ')' ','
%token '+' '-' '*' '/' '<' '>' EqEq

// precedence rules
%left EqEq
%left '>' '<'
%left '+' '-'
%left '*' '/'

%%

program : topStmts1 { bisonProgramResult = new Program(textPos(@1), cast<List<Node>>($1)); };

topStmts1 : topStmt           { $$ = new List<Node>(textPos(@1), $1); }
          | topStmt topStmts1 { cast<List<Node>>($2)->cons($1); $$ = $2; };

topStmt : expr NEWLINE { $$ = $1; }
        | block        { $$ = $1; };


expr
    : INTEGER              { $$ = $1; }
    | FLOATING             { $$ = $1; }
    | '(' expr ')'         { $$ = $2; }
    | ident                { $$ = $1; }
    | ident '(' exprs1 ')' { $$ = new Call(textPos(@2), cast<Ident>($1)->ident, cast<List<Node>>($3)); }
    | ident '(' ')'        { $$ = new Call(textPos(@2), cast<Ident>($1)->ident, new List<Node>(textPos(@2))); }
    | '-' expr             { $$ = new Prefix(textPos(@1), Minus, ($2)); }
    | expr '+' expr        { $$ = new Infix(textPos(@2), $1, Plus, $3); }
    | expr '-' expr        { $$ = new Infix(textPos(@2), $1, Minus, $3); }
    | expr '*' expr        { $$ = new Infix(textPos(@2), $1, Times, $3); }
    | expr '/' expr        { $$ = new Infix(textPos(@2), $1, Divide, $3); }
    | expr '<' expr        { $$ = new Infix(textPos(@2), $1, LT, $3); }
    | expr '>' expr        { $$ = new Infix(textPos(@2), $1, GT, $3); }
    | expr EqEq expr       { $$ = new Infix(textPos(@2), $1, EqEq, $3); };

    // error ast node can go here
    //| error               { llvm::errs() << "syntax error in expr\n"; yyclearin; };


line
    : Return expr        { $$ = new Return(textPos(@1), $2); }
    | ident '=' expr     { $$ = new Set(textPos(@2), cast<Ident>($1)->ident, $3); }
    | Let ident '=' expr { $$ = new Let(textPos(@1), cast<Ident>($2)->ident, $4); };

block
    : fn ident '(' idents ')' INDENT stmts1 DEDENT
        { $$ = new FnDef(textPos(@1), cast<Ident>($2), cast<List<Ident>>($4), cast<List<Node>>($7)); }
    | If expr INDENT stmts1 DEDENT
        { $$ = new If(textPos(@1), $2, cast<List<Node>>($4), new List<Node>(textPos(@1))); }
    | If expr INDENT stmts1 DEDENT Else INDENT stmts1 DEDENT
        { $$ = new If(textPos(@1), $2, cast<List<Node>>($4), cast<List<Node>>($8)); }
    | For expr INDENT stmts1 DEDENT 
        { $$ = new For(textPos(@1), $2, cast<List<Node>>($4)); };

stmts1
    : line NEWLINE        { $$ = new List<Node>(textPos(@1), $1); }
    | block               { $$ = new List<Node>(textPos(@1), $1); }
    | line NEWLINE stmts1 { cast<List<Node>>($3)->cons($1); $$ = $3; }
    | block        stmts1 { cast<List<Node>>($2)->cons($1); $$ = $2; };


idents
    : idents1 { $$ = $1; }
    |         { $$ = new List<Ident>(TextPos(0, 0, 0)); };
idents1
    : ident             { $$ = new List<Ident>(textPos(@1), cast<Ident>($1)); }
    | ident ',' idents1 { cast<List<Ident>>($3)->cons(cast<Ident>($1)); $$ = $3; };


exprs1
    : expr            { $$ = new List<Node>(textPos(@1), $1); }
    | expr ',' exprs1 { cast<List<Node>>($3)->cons($1); $$ = $3; };
    
%%

namespace yy {
// provide a definition for the virtual error member
void parser::error(const location_type& loc, const string& msg) {
    cerr << loc.begin.line << ":" << loc.begin.column << ": " << msg << endl;
}
}

