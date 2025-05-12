#ifndef TYPES_H
#define TYPES_H

#include "common.h"

struct lval;
struct lenv;
typedef struct lval* (*lbuiltin)(struct lenv*, struct lval*);

typedef enum {
    LVAL_ERR,
    LVAL_NUM,
    LVAL_SYM,
    LVAL_STR,
    LVAL_FUN,
    LVAL_SEXPR,
    LVAL_QEXPR
} lval_type;

struct lval {
    lval_type type;

    long num;
    char* err;
    char* sym;
    char* str;

    lbuiltin builtin;
    struct lenv* env;
    struct lval* formals;
    struct lval* body;

    int count;
    struct lval** cell;
};

struct lenv {
    struct lenv* par;
    int count;
    char** syms;
    struct lval** vals;
};

struct lval* lval_num(long x);
struct lval* lval_err(char* fmt, ...);
struct lval* lval_sym(char* s);
struct lval* lval_str(char* s);
struct lval* lval_builtin(lbuiltin func);
struct lval* lval_lambda(struct lval* formals, struct lval* body);
struct lval* lval_sexpr(void);
struct lval* lval_qexpr(void);

void lval_del(struct lval* v);
struct lval* lval_add(struct lval* v, struct lval* x);
struct lval* lval_copy(struct lval* v);

void lval_print(struct lval* v);
void lval_println(struct lval* v);
void lval_expr_print(struct lval* v, char open, char close);
char* ltype_name(lval_type t);

struct lenv* lenv_new(void);
void lenv_del(struct lenv* e);
struct lval* lenv_get(struct lenv* e, struct lval* k);
void lenv_put(struct lenv* e, struct lval* k, struct lval* v);
void lenv_def(struct lenv* e, struct lval* k, struct lval* v);
struct lenv* lenv_copy(struct lenv* e);

void lenv_add_builtins(struct lenv* e);

#endif // TYPES_H
