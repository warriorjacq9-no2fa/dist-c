#include <stdlib.h>
#include <string.h>
#include "ast.h"

struct ast* ast_new() {
    struct ast* ast = malloc(sizeof(struct ast));
    memset(ast, 0, sizeof(struct ast));
    return ast;
}

void ast_free(struct ast* ast) {
    switch(ast->type) {
        case AST_NUMBER: break;
        case AST_IDENT:
            free(ast->ident.value);
            break;
        case AST_BINARY:
            if(ast->binary.left) ast_free(ast->binary.left);
            if(ast->binary.right) ast_free(ast->binary.right);
            break;
    }

    free(ast);
}