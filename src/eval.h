#ifndef EVAL_H
#define EVAL_H

#include "types.h"

struct lval* lval_eval_sexpr(struct lenv* e, struct lval* v);
struct lval* lval_eval(struct lenv* e, struct lval* v);

struct lval* builtin_op(struct lenv* e, struct lval* a, char* op);
struct lval* builtin_add(struct lenv* e, struct lval* a);
struct lval* builtin_sub(struct lenv* e, struct lval* a);
struct lval* builtin_mul(struct lenv* e, struct lval* a);
struct lval* builtin_div(struct lenv* e, struct lval* a);
struct lval* builtin_mod(struct lenv* e, struct lval* a);
struct lval* builtin_pow(struct lenv* e, struct lval* a);

struct lval* builtin_head(struct lenv* e, struct lval* a);
struct lval* builtin_tail(struct lenv* e, struct lval* a);
struct lval* builtin_list(struct lenv* e, struct lval* a);
struct lval* builtin_eval(struct lenv* e, struct lval* a);
struct lval* builtin_join(struct lenv* e, struct lval* a);
struct lval* builtin_cons(struct lenv* e, struct lval* a);
struct lval* builtin_len(struct lenv* e, struct lval* a);
struct lval* builtin_init(struct lenv* e, struct lval* a);

struct lval* builtin_def(struct lenv* e, struct lval* a);
struct lval* builtin_put(struct lenv* e, struct lval* a);
struct lval* builtin_lambda(struct lenv* e, struct lval* a);

struct lval* builtin_gt(struct lenv* e, struct lval* a);
struct lval* builtin_lt(struct lenv* e, struct lval* a);
struct lval* builtin_ge(struct lenv* e, struct lval* a);
struct lval* builtin_le(struct lenv* e, struct lval* a);
struct lval* builtin_eq(struct lenv* e, struct lval* a);
struct lval* builtin_ne(struct lenv* e, struct lval* a);

struct lval* builtin_if(struct lenv* e, struct lval* a);

struct lval* builtin_load(struct lenv* e, struct lval* a);

struct lval* builtin_print(struct lenv* e, struct lval* a);
struct lval* builtin_error(struct lenv* e, struct lval* a);

struct lval* lval_call(struct lenv* e, struct lval* f, struct lval* a);

void lenv_add_builtin(struct lenv* e, char* name, lbuiltin func);
void lenv_add_builtins(struct lenv* e);

#endif // EVAL_H
