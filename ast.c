#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "lexer.h"
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

void _ast_print(struct ast* ast, int indent) {
    if(ast == NULL) return;
    switch(ast->type) {
        case AST_NUMBER:
            printf("%d", ast->number.value);
            break;
        case AST_IDENT:
            printf("%s", ast->ident.value);
            break;
        case AST_BINARY:
            if(ast->binary.tok >= 0 && ast->binary.tok < TOK_EOF)
                printf("(%s\n", token_strings[ast->binary.tok]);
            else
                printf("(\n");
            for(int i = 0; i < indent; i++) printf(" ");
            _ast_print(ast->binary.left, indent + 4);
            printf("\n");
            for(int i = 0; i < indent; i++) printf(" ");
            _ast_print(ast->binary.right,  indent + 4);
            printf("\n");
            for(int i = 0; i < indent - 4; i++) printf(" ");
            printf(")");
            break;
    }
}

void ast_print(struct ast* ast) {
    _ast_print(ast, 4);
    printf("\n");
}