%{
#include <stdio.h>
#include <stdlib.h>
#include "types.h"

int yylex(void);
void yyerror(const char *s);
extern int yylineno;
extern char* yytext;

struct lval* ast_root = NULL;

%}

%union {
    struct lval* val;
    char* str;
}

%token <num> NUMBER
%token <sym> SYMBOL
%token <str> STRING
%token LPAREN RPAREN LBRACE RBRACE QUOTE
%token NEWLINE UNKNOWN_TOKEN YYEOF

%type <val> program expr sexpr qexpr list items item

// %left '+' '-'
// %left '*' '/'

%%

program:
    /* empty */         { $$ = lval_sexpr(); ast_root = $$; } 
    | program expr NEWLINE { ast_root = lval_add($1, $2); $$ = ast_root; }
    | program expr YYEOF  { ast_root = lval_add($1, $2); $$ = ast_root; }
    | program NEWLINE   { $$ = $1; }
    ;

expr:
    item                { $$ = $1; }
    | sexpr             { $$ = $1; }
    | qexpr             { $$ = $1; }
    | QUOTE expr        { 
                          struct lval* q = lval_sym("quote");
                          struct lval* s = lval_sexpr();
                          s = lval_add(s, q);
                          s = lval_add(s, $2);
                          $$ = s;
                        }
    ;

sexpr: 
    LPAREN list RPAREN  { $$ = $2; $$->type = LVAL_SEXPR; } 
    ;

qexpr:
    LBRACE list RBRACE  { $$ = $2; $$->type = LVAL_QEXPR; }
    ;

list:
    /* empty */         { $$ = lval_sexpr(); }
    | items             { $$ = $1; }
    ;

items:
    expr                { $$ = lval_sexpr(); $$ = lval_add($$, $1); }
    | items expr        { $$ = lval_add($1, $2); }
    ;

item:
    NUMBER              { $$ = lval_num($1); }
    | SYMBOL              { $$ = lval_sym($1); free($1); }
    | STRING              { $$ = lval_str($1); free($1); }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Parse Error (line %d near '%s'): %s\n", yylineno, yytext, s);
}
