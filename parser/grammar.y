%require "3.2"
%define parse.error verbose
%define api.value.type { Sparse<ast::Node>::Key }
%locations

%{
#include "sparse.h"
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

Sparse<ast::Node>                     astResult;
std::optional<Sparse<ast::Node>::Key> astResultProgramKey;

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


program : topStmts1 { astResultProgramKey = astResult.insert(Program(textPos(@1), $1)); };

topStmts1 : topStmt           { $$ = astResult.insert(List(textPos(@1), $1)); }
          | topStmt topStmts1 { ((ast::List*)&astResult.at($2))->cons($1); $$ = $2; };

topStmt : expr NEWLINE { $$ = $1; }
        | block        { $$ = $1; };


expr
    : INTEGER              { $$ = $1; }
    | '-' expr             { $$ = astResult.insert(Prefix(textPos(@1), Minus, $2)); }
    | expr '+' expr        { $$ = astResult.insert(Infix(textPos(@2), $1, Plus, $3)); }
    | expr '-' expr        { $$ = astResult.insert(Infix(textPos(@2), $1, Minus, $3)); }
    | expr '*' expr        { $$ = astResult.insert(Infix(textPos(@2), $1, Times, $3)); }
    | expr '/' expr        { $$ = astResult.insert(Infix(textPos(@2), $1, Divide, $3)); }
    | expr '<' expr        { $$ = astResult.insert(Infix(textPos(@2), $1, LT, $3)); }
    | expr '>' expr        { $$ = astResult.insert(Infix(textPos(@2), $1, GT, $3)); }
    | expr EqEq expr       { $$ = astResult.insert(Infix(textPos(@2), $1, EqEq, $3)); }
    | '(' expr ')'         { $$ = $2; }
    | ident                { $$ = $1; }
    | ident '(' exprs1 ')' { $$ = astResult.insert(Call(textPos(@2), ((ast::Ident*)&astResult.at($1))->ident, $3)); }
    | ident '(' ')'        { $$ = astResult.insert(Call(textPos(@2), ((ast::Ident*)&astResult.at($1))->ident, astResult.insert(List(textPos(@2))))); };

//    // error ast node can go here
//    //| error               { llvm::errs() << "syntax error in expr\n"; yyclearin; };


line
    : Return expr        { $$ = astResult.insert(Return(textPos(@1), $2)); }
    | ident '=' expr     { $$ = astResult.insert(Set(textPos(@2), ((ast::Ident*)&astResult.at($1))->ident, $3)); }
    | Let ident '=' expr { $$ = astResult.insert(Let(textPos(@1), ((ast::Ident*)&astResult.at($2))->ident, $4)); };

block
    : fn ident '(' idents ')' INDENT stmts1 DEDENT
        { $$ = astResult.insert(FnDef(textPos(@1), ((ast::Ident*)&astResult.at($2))->ident, $4, $7)); }
    | If expr INDENT stmts1 DEDENT
        { $$ = astResult.insert(If(textPos(@1), $2, $4, astResult.insert(List(textPos(@1))))); }
    | If expr INDENT stmts1 DEDENT Else INDENT stmts1 DEDENT
        { $$ = astResult.insert(If(textPos(@1), $2, $4, $8)); }
    | For expr INDENT stmts1 DEDENT 
        { $$ = astResult.insert(For(textPos(@1), $2, $4)); };

stmts1
    : line NEWLINE        { $$ = astResult.insert(List(textPos(@1), $1)); }
    | block               { $$ = astResult.insert(List(textPos(@1), $1)); }
    | line NEWLINE stmts1 { ((ast::List*)&astResult.at($3))->cons($1); $$ = $3; }
    | block        stmts1 { ((ast::List*)&astResult.at($2))->cons($1); $$ = $2; };


idents
    : idents1 { $$ = $1; }
    |         { $$ = astResult.insert(List(TextPos(0, 0, 0))); };
idents1
    : ident             { $$ = astResult.insert(List(textPos(@1), $1)); }
    | ident ',' idents1 { ((ast::List*)&astResult.at($3))->cons($1); $$ = $3; };


exprs1
    : expr            { $$ = astResult.insert(List(textPos(@1), $1)); }
    | expr ',' exprs1 { ((ast::List*)&astResult.at($3))->cons($1); $$ = $3; };
    
%%

namespace yy {
// provide a definition for the virtual error member
void parser::error(const location_type& loc, const string& msg) {
    cerr << loc.begin.line << ":" << loc.begin.column << ": " << msg << endl;
}
}

