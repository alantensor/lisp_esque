#include "eval.h"
#include "parser.tab.h"

extern int yyparse(void);
extern struct lval* ast_root;
extern FILE* yyin;

#define LASSERT(args, cond, fmt, ...) \
    if (!(cond)) { \
        struct lval* err = lval_err(fmt, ##__VA_ARGS__); \
        lval_del(args); \
        return err; \
    }

#define LASSERT_TYPE(func, args, index, expect) \
    LASSERT(args, args->cell[index]->type == expect, \
        "Function \'%s\' passed incorrect type for argument %i. Got %s, Expected %s.", \
        func, index, ltype_name(args->cell[index]->type), ltype_name(expect))

#define LASSERT_NUM_ARGS(func, args, num) \
    LASSERT(args, args->count == num, \
        "Function \'%s\' passed incorrect number of arguments. Got %i, Expected %i.", \
        func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index) \
    LASSERT(args, args->cell[index]->count != 0, \
        "Function \'%s\' passed {} for argument %i.", func, index)


struct lval* lval_eval(struct lenv* e, struct lval* v) {
    if (v->type == LVAL_SYM) {
        struct lval* x = lenv_get(e, v);
        lval_del(v);
        return x;
    }
    if (v->type == LVAL_SEXPR) {
        return lval_eval_sexpr(e, v);
    }
    return v;
}

struct lval* lval_eval_sexpr(struct lenv* e, struct lval* v) {
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(e, v->cell[i]);
    }
    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
    }

    if (v->count == 0) { return v; }
    if (v->count == 1) { return lval_take(v, 0); }

    struct lval* f = lval_pop(v, 0);
    if (f->type != LVAL_FUN) {
        struct lval* err = lval_err(
            "S-Expression starts with incorrect type. "
            "Got %s, Expected %s.",
            ltype_name(f->type), ltype_name(LVAL_FUN));
        lval_del(f); lval_del(v);
        return err;
    }

    struct lval* result = lval_call(e, f, v); // v now contains only arguments
    lval_del(f);
    return result;
}

struct lval* lval_call(struct lenv* e, struct lval* f, struct lval* a) {
    if (f->builtin) { return f->builtin(e, a); }

    int given = a->count;
    int total = f->formals->count;

    while (a->count) {
        if (f->formals->count == 0) {
            lval_del(a);
            return lval_err("Function passed too many arguments. "
                            "Got %i, Expected %i.", given, total);
        }

        struct lval* sym = lval_pop(f->formals, 0);

        if (strcmp(sym->sym, "&") == 0) {
            if (f->formals->count != 1) {
                lval_del(a); lval_del(sym); lval_del(f->formals);
                return lval_err("Function format invalid. "
                                "Symbol '&' not followed by single symbol.");
            }
            struct lval* nsym = lval_pop(f->formals, 0);
            lenv_put(f->env, nsym, builtin_list(e, a)); // 'a' becomes the list of remaining args
            lval_del(sym); lval_del(nsym);
            break; 
        }

        struct lval* val = lval_pop(a, 0);
        lenv_put(f->env, sym, val);
        lval_del(sym); lval_del(val);
    }

    lval_del(a); // Arguments are now bound or handled

    if (f->formals->count > 0 && strcmp(f->formals->cell[0]->sym, "&") == 0) {
        // Varargs symbol present, but no more arguments were given. Bind to empty list.
        if (f->formals->count != 2) { // Should be '&' and one symbol name
             lval_del(f->formals);
             return lval_err("Function format invalid. Symbol '&' not followed by single symbol for varargs.");
        }
        lval_del(lval_pop(f->formals, 0)); // Pop '&'
        struct lval* sym = lval_pop(f->formals, 0);
        lenv_put(f->env, sym, lval_qexpr()); // Bind to empty qexpr
        lval_del(sym);
    }


    if (f->formals->count == 0) {
        f->env->par = e; // Set parent env for evaluation context
        return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
    } else {
        return lval_copy(f); // Return partially applied function (or error if not all args bound)
    }
}

struct lval* builtin_op(struct lenv* e, struct lval* a, char* op) {
    for (int i = 0; i < a->count; i++) {
        LASSERT_TYPE(op, a, i, LVAL_NUM);
    }

    struct lval* x = lval_pop(a, 0);

    if ((strcmp(op, "-") == 0) && a->count == 0) {
        x->num = -x->num;
    }

    while (a->count > 0) {
        struct lval* y = lval_pop(a, 0);
        if (strcmp(op, "+") == 0) { x->num += y->num; }
        if (strcmp(op, "-") == 0) { x->num -= y->num; }
        if (strcmp(op, "*") == 0) { x->num *= y->num; }
        if (strcmp(op, "/") == 0) {
            if (y->num == 0) {
                lval_del(x); lval_del(y);
                x = lval_err("Division By Zero."); break;
            }
            x->num /= y->num;
        }
        if (strcmp(op, "%") == 0) {
            if (y->num == 0) {
                lval_del(x); lval_del(y);
                x = lval_err("Division By Zero (Modulo)."); break;
            }
            x->num %= y->num;
        }
        lval_del(y);
    }
    lval_del(a);
    return x;
}

struct lval* builtin_add(struct lenv* e, struct lval* a) { return builtin_op(e, a, "+"); }
struct lval* builtin_sub(struct lenv* e, struct lval* a) { return builtin_op(e, a, "-"); }
struct lval* builtin_mul(struct lenv* e, struct lval* a) { return builtin_op(e, a, "*"); }
struct lval* builtin_div(struct lenv* e, struct lval* a) { return builtin_op(e, a, "/"); }
struct lval* builtin_mod(struct lenv* e, struct lval* a) { return builtin_op(e, a, "%"); }
struct lval* builtin_pow(struct lenv* e, struct lval* a) { return builtin_op(e, a, "^"); }

struct lval* builtin_head(struct lenv* e, struct lval* a) {
    LASSERT_NUM_ARGS("head", a, 1);
    LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("head", a, 0);

    struct lval* v = lval_take(a, 0);
    while (v->count > 1) { lval_del(lval_pop(v, 1)); }
    return v;
}

struct lval* builtin_tail(struct lenv* e, struct lval* a) {
    LASSERT_NUM_ARGS("tail", a, 1);
    LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("tail", a, 0);

    struct lval* v = lval_take(a, 0);
    lval_del(lval_pop(v, 0));
    return v;
}

struct lval* builtin_list(struct lenv* e, struct lval* a) {
    a->type = LVAL_QEXPR;
    return a;
}

struct lval* builtin_eval(struct lenv* e, struct lval* a) {
    LASSERT_NUM_ARGS("eval", a, 1);
    LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

    struct lval* x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}

struct lval* lval_join_qexpr(struct lval* x, struct lval* y) {
    while (y->count) {
        x = lval_add(x, lval_pop(y, 0));
    }
    lval_del(y);
    return x;
}

struct lval* builtin_join(struct lenv* e, struct lval* a) {
    for (int i = 0; i < a->count; i++) {
        LASSERT_TYPE("join", a, i, LVAL_QEXPR);
    }
    struct lval* x = lval_pop(a, 0);
    while (a->count) {
        x = lval_join_qexpr(x, lval_pop(a, 0));
    }
    lval_del(a);
    return x;
}

struct lval* builtin_cons(struct lenv* e, struct lval* a) {
    LASSERT_NUM_ARGS("cons", a, 2);
    LASSERT_TYPE("cons", a, 1, LVAL_QEXPR);

    struct lval* x = lval_pop(a, 0);
    struct lval* q = lval_pop(a, 0);
    lval_del(a);

    struct lval* res = lval_qexpr();
    res = lval_add(res, x);

    while (q->count) {
        res = lval_add(res, lval_pop(q, 0));
    }
    lval_del(q);
    return res;
}

struct lval* builtin_len(struct lenv* e, struct lval* a) {
    LASSERT_NUM_ARGS("len", a, 1);
    LASSERT_TYPE("len", a, 0, LVAL_QEXPR);
    struct lval* q = lval_take(a,0);
    long count = q->count;
    lval_del(q);
    return lval_num(count);
}

struct lval* builtin_init(struct lenv* e, struct lval* a) {
    LASSERT_NUM_ARGS("init", a, 1);
    LASSERT_TYPE("init", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("init", a, 0);

    struct lval* v = lval_take(a, 0);
    lval_del(lval_pop(v, v->count - 1));
    return v;
}

struct lval* builtin_var(struct lenv* e, struct lval* a, char* func) {
    LASSERT_TYPE(func, a, 0, LVAL_QEXPR);

    struct lval* syms = a->cell[0];
    for (int i = 0; i < syms->count; i++) {
        LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
            "Function \'%s\' cannot define non-symbol. Got %s, Expected %s.", 
            func, ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
    }

    LASSERT(a, (syms->count == a->count - 1),
        "Function \'%s\' passed too many arguments for symbols. Got %i, Expected %i.", 
        func, syms->count, a->count - 1);

    for (int i = 0; i < syms->count; i++) {
        if (strcmp(func, "def") == 0) { lenv_def(e, syms->cell[i], a->cell[i+1]); }
        if (strcmp(func, "=")   == 0) { lenv_put(e, syms->cell[i], a->cell[i+1]); }
    }

    lval_del(a);
    return lval_sexpr();
}

struct lval* builtin_def(struct lenv* e, struct lval* a) { return builtin_var(e, a, "def"); }
struct lval* builtin_put(struct lenv* e, struct lval* a) { return builtin_var(e, a, "=");   }

struct lval* builtin_lambda(struct lenv* e, struct lval* a) {
    LASSERT_NUM_ARGS("\\\\", a, 2);
    LASSERT_TYPE("\\\\", a, 0, LVAL_QEXPR);
    LASSERT_TYPE("\\\\", a, 1, LVAL_QEXPR);

    for (int i = 0; i < a->cell[0]->count; i++) {
        LASSERT(a, (a->cell[0]->cell[i]->type == LVAL_SYM),
            "Cannot define non-symbol. Got %s, Expected %s.",
            ltype_name(a->cell[0]->cell[i]->type), ltype_name(LVAL_SYM));
    }

    struct lval* formals = lval_pop(a, 0);
    struct lval* body = lval_pop(a, 0);
    lval_del(a);

    return lval_lambda(formals, body);
}

struct lval* builtin_ord(struct lenv* e, struct lval* a, char* op) {
    LASSERT_NUM_ARGS(op, a, 2);
    LASSERT_TYPE(op, a, 0, LVAL_NUM);
    LASSERT_TYPE(op, a, 1, LVAL_NUM);

    int r;
    long n1 = a->cell[0]->num;
    long n2 = a->cell[1]->num;
    lval_del(a);

    if (strcmp(op, ">") == 0)  { r = (n1 > n2);  }
    else if (strcmp(op, "<") == 0)  { r = (n1 < n2);  }
    else if (strcmp(op, ">=") == 0) { r = (n1 >= n2); }
    else if (strcmp(op, "<=") == 0) { r = (n1 <= n2); }
    else { return lval_err("Unknown comparison operator %s", op); }

    return lval_num(r);
}

struct lval* builtin_gt(struct lenv* e, struct lval* a) { return builtin_ord(e, a, ">"); }
struct lval* builtin_lt(struct lenv* e, struct lval* a) { return builtin_ord(e, a, "<"); }
struct lval* builtin_ge(struct lenv* e, struct lval* a) { return builtin_ord(e, a, ">="); }
struct lval* builtin_le(struct lenv* e, struct lval* a) { return builtin_ord(e, a, "<="); }

int lval_eq(struct lval* x, struct lval* y) {
    if (x->type != y->type) { return 0; }
    switch (x->type) {
        case LVAL_NUM: return (x->num == y->num);
        case LVAL_ERR: return (strcmp(x->err, y->err) == 0);
        case LVAL_SYM: return (strcmp(x->sym, y->sym) == 0);
        case LVAL_STR: return (strcmp(x->str, y->str) == 0);
        case LVAL_FUN:
            if (x->builtin || y->builtin) { return x->builtin == y->builtin; }
            else { return lval_eq(x->formals, y->formals) && lval_eq(x->body, y->body); }
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            if (x->count != y->count) { return 0; }
            for (int i = 0; i < x->count; i++) {
                if (!lval_eq(x->cell[i], y->cell[i])) { return 0; }
            }
            return 1;
        break;
    }
    return 0;
}

struct lval* builtin_cmp(struct lenv* e, struct lval* a, char* op) {
    LASSERT_NUM_ARGS(op, a, 2);
    int r;
    if (strcmp(op, "==") == 0) { r =  lval_eq(a->cell[0], a->cell[1]); }
    else if (strcmp(op, "!=") == 0) { r = !lval_eq(a->cell[0], a->cell[1]); }
    else { lval_del(a); return lval_err("Unknown equality operator %s", op); }
    lval_del(a);
    return lval_num(r);
}

struct lval* builtin_eq(struct lenv* e, struct lval* a) { return builtin_cmp(e, a, "=="); }
struct lval* builtin_ne(struct lenv* e, struct lval* a) { return builtin_cmp(e, a, "!="); }

struct lval* builtin_if(struct lenv* e, struct lval* a) {
    LASSERT_NUM_ARGS("if", a, 3);
    LASSERT_TYPE("if", a, 0, LVAL_NUM);
    LASSERT_TYPE("if", a, 1, LVAL_QEXPR);
    LASSERT_TYPE("if", a, 2, LVAL_QEXPR);

    struct lval* cond_val = lval_pop(a, 0);
    struct lval* true_branch = lval_pop(a, 0);
    struct lval* false_branch = lval_pop(a, 0);
    lval_del(a);

    struct lval* result;
    true_branch->type = LVAL_SEXPR;
    false_branch->type = LVAL_SEXPR;

    if (cond_val->num) {
        result = lval_eval(e, true_branch);
        lval_del(false_branch);
    } else {
        result = lval_eval(e, false_branch);
        lval_del(true_branch);
    }
    lval_del(cond_val);
    return result;
}

struct lval* builtin_load(struct lenv* e, struct lval* a) {
    LASSERT_NUM_ARGS("load", a, 1);
    LASSERT_TYPE("load", a, 0, LVAL_STR);

    // Store the first argument (filename)
    char* filename = a->cell[0]->str;

    // Parse File
    // We need a new ast_root for this parsing instance.
    // The global ast_root is for the REPL line.
    struct lval* file_ast_root = lval_sexpr();
    struct lval* expr = NULL; // To hold individual expressions from the file

    FILE* f = fopen(filename, "r");
    if (!f) {
        lval_del(a);
        return lval_err("Could not load file '%s'", filename);
    }

    // Store old yyin, set new one
    FILE* old_yyin = yyin;
    yyin = f;
    
    // Store old ast_root, set new one for this scope
    struct lval* previous_ast_root_ptr = ast_root; // Save the REPL's ast_root pointer
    ast_root = file_ast_root; // Point global ast_root to our local one for yyparse

    int parse_result = yyparse(); // This will populate our local file_ast_root

    fclose(f);
    yyin = old_yyin; // Restore original yyin
    ast_root = previous_ast_root_ptr; // Restore original ast_root pointer

    lval_del(a); // Delete the argument list (filename string)

    if (parse_result != 0 || !file_ast_root) {
        if(file_ast_root) lval_del(file_ast_root);
        return lval_err("Syntax error in loaded file '%s'.", filename);
    }

    lval* result_val = lval_sexpr(); // Default to empty Sexpr if file is empty or only comments

    // Evaluate each expression in the file
    while (file_ast_root->count) {
        expr = lval_pop(file_ast_root, 0);
        lval* eval_res = lval_eval(e, expr); // expr is consumed by lval_eval

        if (eval_res->type == LVAL_ERR) {
            lval_del(result_val); // clean up previous result if any
            result_val = eval_res; // Store the error
            break; // Stop on error
        }
        lval_del(result_val); // Delete previous non-error result
        result_val = eval_res; // Store current result
    }

    lval_del(file_ast_root); // Clean up the AST from the file
    return result_val; // Return the last evaluated expression or error
}

struct lval* builtin_print(struct lenv* e, struct lval* a) {
    for (int i = 0; i < a->count; i++) {
        lval_print(a->cell[i]);
        if (i < a->count - 1) { putchar(' '); }
    }
    putchar('\n');
    lval_del(a);
    return lval_sexpr();
}

struct lval* builtin_error(struct lenv* e, struct lval* a) {
    LASSERT_NUM_ARGS("error", a, 1);
    LASSERT_TYPE("error", a, 0, LVAL_STR);

    struct lval* err = lval_err(a->cell[0]->str);
    lval_del(a);
    return err;
}

void lenv_add_builtin(struct lenv* e, char* name, lbuiltin func) {
    struct lval* k = lval_sym(name);
    struct lval* v = lval_builtin(func);
    lenv_put(e, k, v);
    lval_del(k); lval_del(v);
}

void lenv_add_builtins(struct lenv* e) {
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);
    lenv_add_builtin(e, "cons", builtin_cons);
    lenv_add_builtin(e, "len",  builtin_len);
    lenv_add_builtin(e, "init", builtin_init);

    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);
    lenv_add_builtin(e, "%", builtin_mod);
    lenv_add_builtin(e, "^", builtin_pow);

    lenv_add_builtin(e, "def", builtin_def);
    lenv_add_builtin(e, "=",   builtin_put);
    lenv_add_builtin(e, "\\\\",  builtin_lambda);

    lenv_add_builtin(e, ">",  builtin_gt);
    lenv_add_builtin(e, "<",  builtin_lt);
    lenv_add_builtin(e, ">=", builtin_ge);
    lenv_add_builtin(e, "<=", builtin_le);
    lenv_add_builtin(e, "==", builtin_eq);
    lenv_add_builtin(e, "!=", builtin_ne);

    lenv_add_builtin(e, "if", builtin_if);

    lenv_add_builtin(e, "load", builtin_load);

    lenv_add_builtin(e, "print", builtin_print);
    lenv_add_builtin(e, "error", builtin_error);

    // `quote` is a special form handled by parser usually
}

// Helper to pop an lval from a list
struct lval* lval_pop(struct lval* v, int i) {
    struct lval* x = v->cell[i];
    memmove(&v->cell[i], &v->cell[i+1], sizeof(struct lval*) * (v->count-i-1));
    v->count--;
    v->cell = realloc(v->cell, sizeof(struct lval*) * v->count);
    return x;
}

// Helper to take an lval (pop and delete containing list)
struct lval* lval_take(struct lval* v, int i) {
    struct lval* x = lval_pop(v, i);
    lval_del(v);
    return x;
}
