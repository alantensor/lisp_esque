#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>   // For fmod

void yyerror(const char *s);

struct lval;
struct lenv;

#endif // COMMON_H
