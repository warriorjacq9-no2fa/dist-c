#include "ast.h"
#include <stdlib.h>

AST* ast_new(AST ast) {
    AST* ptr = malloc(sizeof(AST));
    if(ptr) *ptr = ast;
    return ptr;
}

/* @ADDNEW */
void ast_free(AST *ptr) {
    AST ast = *ptr;
    switch (ast.tag) {
        case AST_NUMBER: {
            struct AST_NUMBER data = ast.data.AST_NUMBER;
            break;
        }
        case AST_ADD: {
            struct AST_ADD data = ast.data.AST_ADD;
            ast_free(data.left);
            ast_free(data.right);
            break;
        }
        case AST_MUL: {
            struct AST_MUL data = ast.data.AST_MUL;
            ast_free(data.left);
            ast_free(data.right);
            break;
        }
    } 
    free(ptr);
}   