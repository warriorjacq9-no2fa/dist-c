#include "ast.h"
#include <stdio.h>
#include <stdlib.h>

char* ast_names[] = {
    "",
    "NUMBER",
    "SUM",
    "MUL"
};

void ast_free(AST* ast) {
    if(ast == NULL) return;

    ast_free(ast->left);
    ast_free(ast->right);

    free(ast->token);
    free(ast);
}

void _ast_print(AST* ast) {
    switch(ast->token_enum) {
        case AST_NUMBER: {
            printf("%s", ast->token);
            break;
        }
        case AST_SUM: {
            printf("(+ ");
            _ast_print(ast->left);
            printf(" ");
            _ast_print(ast->right);
            printf(")");
            break;
        }
        case AST_MUL: {
            printf("(* ");
            _ast_print(ast->left);
            printf(" ");
            _ast_print(ast->right);
            printf(")");
            break;
        }
        default: break;
    }
}

void ast_print(AST* ast) {
    _ast_print(ast);
    printf("\n");
}