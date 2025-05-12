#include <stdio.h>
#include <stdlib.h>

#ifdef USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "common.h"
#include "types.h"
#include "eval.h"
#include "parser.tab.h"

extern FILE *yyin;
extern int yyparse(void);
extern struct lval* ast_root;

int main(int argc, char** argv) {
    printf("MyLisp Version 0.0.1\n");
    printf("Press Ctrl+c or type \"quit\" to Exit\n\n");

    struct lenv* env = lenv_new();
    lenv_add_builtins(env);

    if (argc == 1) {
        while (1) {
            char* input = NULL;

#ifdef USE_READLINE
            input = readline("mylisp> ");
            if (!input) {
                printf("Exiting.\n");
                break;
            }
            add_history(input);
#else
            char buffer[2048];
            printf("mylisp> ");
            fflush(stdout);
            if (!fgets(buffer, sizeof(buffer), stdin)) {
                printf("Exiting.\n");
                break;
            }
            buffer[strcspn(buffer, "\n")] = 0;
            input = malloc(strlen(buffer) + 1);
            strcpy(input, buffer);
#endif

            if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
                printf("Exiting.\n");
                free(input);
                break;
            }
            
            if (strlen(input) == 0) {
                free(input);
                continue;
            }

            char* input_with_newline = malloc(strlen(input) + 2);
            sprintf(input_with_newline, "%s\n", input);

            yyin = fmemopen(input_with_newline, strlen(input_with_newline), "r");
            if (!yyin) {
                perror("fmemopen failed");
                free(input);
                free(input_with_newline);
                continue;
            }

            ast_root = lval_sexpr();
            int parse_result = yyparse();
            fclose(yyin);
            yyin = stdin;

            free(input_with_newline);

            if (parse_result == 0 && ast_root && ast_root->count > 0) {
                struct lval* eval_result = NULL;
                for (int i = 0; i < ast_root->count; i++) {
                    struct lval* expr_to_eval = lval_copy(ast_root->cell[i]);
                    if (eval_result) { lval_del(eval_result); }
                    eval_result = lval_eval(env, expr_to_eval);
                    
                    if (i == ast_root->count -1) {
                         lval_println(eval_result);
                    }
                    lval_del(eval_result);
                }
            } else if (parse_result != 0) {
                // yyerror already printed a message
            } else {
                // No input or parse did not produce a result.
            }
            
            if (ast_root) {
                lval_del(ast_root);
                ast_root = NULL;
            }
            free(input);
        }
    } else {
        for (int i = 1; i < argc; i++) {
            struct lval* args = lval_add(lval_sexpr(), lval_str(argv[i]));
            struct lval* result = builtin_load(env, args);
            if (result->type == LVAL_ERR) {
                lval_println(result);
            }
            lval_del(result);
        }
    }

    lenv_del(env);

    return 0;
}
