#include "types.h"
#include "eval.h" 

struct lval* lval_num(long x) {
    struct lval* v = malloc(sizeof(struct lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

struct lval* lval_err(char* fmt, ...) {
    struct lval* v = malloc(sizeof(struct lval));
    v->type = LVAL_ERR;
    va_list va;
    va_start(va, fmt);
    v->err = malloc(512);
    vsnprintf(v->err, 511, fmt, va);
    v->err = realloc(v->err, strlen(v->err) + 1);
    va_end(va);
    return v;
}

struct lval* lval_sym(char* s) {
    struct lval* v = malloc(sizeof(struct lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

struct lval* lval_str(char* s) {
    struct lval* v = malloc(sizeof(struct lval));
    v->type = LVAL_STR;
    v->str = malloc(strlen(s) + 1);
    strcpy(v->str, s);
    return v;
}

struct lval* lval_builtin(lbuiltin func) {
    struct lval* v = malloc(sizeof(struct lval));
    v->type = LVAL_FUN;
    v->builtin = func;
    return v;
}

struct lval* lval_lambda(struct lval* formals, struct lval* body) {
    struct lval* v = malloc(sizeof(struct lval));
    v->type = LVAL_FUN;
    v->builtin = NULL;
    v->env = lenv_new();
    v->formals = formals;
    v->body = body;
    return v;
}

struct lval* lval_sexpr(void) {
    struct lval* v = malloc(sizeof(struct lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

struct lval* lval_qexpr(void) {
    struct lval* v = malloc(sizeof(struct lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

void lval_del(struct lval* v) {
    switch (v->type) {
        case LVAL_NUM: break;
        case LVAL_ERR: free(v->err); break;
        case LVAL_SYM: free(v->sym); break;
        case LVAL_STR: free(v->str); break;
        case LVAL_FUN:
            if (!v->builtin) {
                lenv_del(v->env);
                lval_del(v->formals);
                lval_del(v->body);
            }
            break;
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            for (int i = 0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }
            free(v->cell);
            break;
    }
    free(v);
}

struct lval* lval_add(struct lval* v, struct lval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(struct lval*) * v->count);
    v->cell[v->count - 1] = x;
    return v;
}

struct lval* lval_copy(struct lval* v) {
    struct lval* x = malloc(sizeof(struct lval));
    x->type = v->type;
    switch (v->type) {
        case LVAL_NUM: x->num = v->num; break;
        case LVAL_ERR: x->err = malloc(strlen(v->err) + 1); strcpy(x->err, v->err); break;
        case LVAL_SYM: x->sym = malloc(strlen(v->sym) + 1); strcpy(x->sym, v->sym); break;
        case LVAL_STR: x->str = malloc(strlen(v->str) + 1); strcpy(x->str, v->str); break;
        case LVAL_FUN:
            if (v->builtin) {
                x->builtin = v->builtin;
            } else {
                x->builtin = NULL;
                x->env = lenv_copy(v->env);
                x->formals = lval_copy(v->formals);
                x->body = lval_copy(v->body);
            }
            break;
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            x->count = v->count;
            x->cell = malloc(sizeof(struct lval*) * x->count);
            for (int i = 0; i < x->count; i++) {
                x->cell[i] = lval_copy(v->cell[i]);
            }
            break;
    }
    return x;
}

void lval_print_expr_contents(struct lval* v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {
        lval_print(v->cell[i]);
        if (i != (v->count - 1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

void lval_print_str(struct lval* v) {
    char* escaped = malloc(strlen(v->str) * 2 + 3);
    char* p = escaped;
    *p++ = '"';
    for (int i = 0; v->str[i] != '\0'; i++) {
        switch (v->str[i]) {
            case '\n': *p++ = '\\'; *p++ = 'n'; break;
            case '\t': *p++ = '\\'; *p++ = 't'; break;
            case '\\': *p++ = '\\'; *p++ = '\\'; break;
            case '"':  *p++ = '\\'; *p++ = '"'; break;
            default:   *p++ = v->str[i]; break;
        }
    }
    *p++ = '"';
    *p++ = '\0';
    printf("%s", escaped);
    free(escaped);
}

void lval_print(struct lval* v) {
    switch (v->type) {
        case LVAL_NUM:   printf("%li", v->num); break;
        case LVAL_ERR:   printf("Error: %s", v->err); break;
        case LVAL_SYM:   printf("%s", v->sym); break;
        case LVAL_STR:   lval_print_str(v); break;
        case LVAL_FUN:
            if (v->builtin) {
                printf("<builtin>");
            } else {
                printf("(lambda ");
                lval_print(v->formals);
                putchar(' ');
                lval_print(v->body);
                putchar(')');
            }
            break;
        case LVAL_SEXPR: lval_print_expr_contents(v, '(', ')'); break;
        case LVAL_QEXPR: lval_print_expr_contents(v, '{', '}'); break;
    }
}

void lval_println(struct lval* v) {
    lval_print(v);
    putchar('\n');
}

char* ltype_name(lval_type t) {
    switch(t) {
        case LVAL_FUN: return "Function";
        case LVAL_NUM: return "Number";
        case LVAL_ERR: return "Error";
        case LVAL_SYM: return "Symbol";
        case LVAL_STR: return "String";
        case LVAL_SEXPR: return "S-Expression";
        case LVAL_QEXPR: return "Q-Expression";
        default: return "Unknown";
    }
}

struct lenv* lenv_new(void) {
    struct lenv* e = malloc(sizeof(struct lenv));
    e->par = NULL;
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}

void lenv_del(struct lenv* e) {
    for (int i = 0; i < e->count; i++) {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e);
}

struct lval* lenv_get(struct lenv* e, struct lval* k) {
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->syms[i], k->sym) == 0) {
            return lval_copy(e->vals[i]);
        }
    }
    if (e->par) {
        return lenv_get(e->par, k);
    }
    return lval_err("Unbound Symbol '%s'", k->sym);
}

void lenv_put(struct lenv* e, struct lval* k, struct lval* v) {
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->syms[i], k->sym) == 0) {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }

    e->count++;
    e->vals = realloc(e->vals, sizeof(struct lval*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);

    e->vals[e->count - 1] = lval_copy(v);
    e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
    strcpy(e->syms[e->count - 1], k->sym);
}

void lenv_def(struct lenv* e, struct lval* k, struct lval* v) {
    while (e->par) { e = e->par; }
    lenv_put(e, k, v);
}

struct lenv* lenv_copy(struct lenv* e) {
    struct lenv* n = malloc(sizeof(struct lenv));
    n->par = e->par;
    n->count = e->count;
    n->syms = malloc(sizeof(char*) * n->count);
    n->vals = malloc(sizeof(struct lval*) * n->count);
    for (int i = 0; i < e->count; i++) {
        n->syms[i] = malloc(strlen(e->syms[i]) + 1);
        strcpy(n->syms[i], e->syms[i]);
        n->vals[i] = lval_copy(e->vals[i]);
    }
    return n;
}

