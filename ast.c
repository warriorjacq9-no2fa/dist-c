#include "ast.h"
#include <stdio.h>
#include <stdlib.h>

AST* ast_new(AST ast) {
    AST* ptr = malloc(sizeof(AST));
    if(ptr) *ptr = ast;
    return ptr;
}

/* @ADDNEW */
void ast_free(AST* ast) {
    switch(ast->tag) {
        case AST_NUMBER: {
            struct AST_NUMBER data = ast->data.AST_NUMBER;
            break;
        }
        case AST_ADD: {
            struct AST_ADD data = ast->data.AST_ADD;
            ast_free(data.left);
            ast_free(data.right);
            break;
        }
        case AST_MUL: {
            struct AST_MUL data = ast->data.AST_MUL;
            ast_free(data.left);
            ast_free(data.right);
            break;
        }
    }
    free(ast);
}

/* @ADDNEW */
void _ast_print(AST* ast) {
    switch(ast->tag) {
        case AST_NUMBER: {
            struct AST_NUMBER data = ast->data.AST_NUMBER;
            printf("%d", data.number);
            break;
        }
        case AST_ADD: {
            struct AST_ADD data = ast->data.AST_ADD;
            printf("(+ ");
            _ast_print(data.left);
            printf(" ");
            _ast_print(data.right);
            printf(")");
            break;
        }
        case AST_MUL: {
            struct AST_MUL data = ast->data.AST_MUL;
            printf("(* ");
            _ast_print(data.left);
            printf(" ");
            _ast_print(data.right);
            printf(")");
            break;
        }
    }
}

void ast_print(AST* ast) {
    _ast_print(ast);
    printf("\n");
}