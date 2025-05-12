%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.tab.h"
#include "types.h"

extern int yywrap() { return 1; }
extern int yylineno;

char* unescape_string(const char* s) {
    int len = strlen(s);
    char* result = malloc(len + 1);
    char* q = result;
    for (int i = 1; i < len - 1; i++) {
        if (s[i] == '\\') {
            i++;
            switch (s[i]) {
                case 'n':  *q++ = '\n'; break;
                case 't':  *q++ = '\t'; break;
                case '\\': *q++ = '\\'; break;
                case '"':  *q++ = '"'; break;
                default:   *q++ = s[i]; break;
            }
        } else {
            *q++ = s[i];
        }
    }
    *q = '\0';
    return result;
}

%}

%option noyywrap nounput noinput

DIGIT    [0-9]
ID_START [a-zA-Z_+\-*\/\\=<>!&%?]
ID_CONT  [a-zA-Z0-9_+\-*\/\\=<>!&%?]
SYMBOL   {ID_START}{ID_CONT}*
STRING   \"(\\.|[^\"\\])*\"
COMMENT  ;[^\n]*

%%

{COMMENT}         { /* Ignore comments */ }
[ \t\r]+          { /* Ignore whitespace */ }
\n                { return NEWLINE; }

"("               { return LPAREN; }
")"               { return RPAREN; }
"{"               { return LBRACE; }
"}"               { return RBRACE; }
"'"               { return QUOTE;  }

{DIGIT}+          {
                    yylval.num = atol(yytext);
                    return NUMBER;
                  }

{SYMBOL}          {
                    yylval.sym = strdup(yytext);
                    return SYMBOL;
                  }

{STRING}          {
                    char* unescaped = unescape_string(yytext);
                    yylval.str = unescaped;
                    return STRING;
                  }

.                 { 
                    char msg[256];
                    snprintf(msg, sizeof(msg), "Unexpected character: %s", yytext);
                    yyerror(msg);
                    return UNKNOWN_TOKEN;
                  }

<<EOF>>           { return YYEOF; }

%%

/*
// This yyerror can be defined in parser.y or a common file.
void yyerror(const char *s) {
    fprintf(stderr, "Lexer Error: %s near line %d, text: %s\n", s, yylineno, yytext);
}
*/
