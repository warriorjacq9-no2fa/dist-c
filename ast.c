#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ast.h"

const char* const op_strings[] = {
    "type",
    "post",
    "x++",
    "x--",
    "call",
    "args",
    "[]",
    "++x",
    "--x",
    "+x",
    "-x",
    "!",
    "~",
    "(cast)",
    "*x",
    "&x",
    "*",
    "/",
    "%",
    "+",
    "-",
    "<<",
    ">>",
    "<",
    "<=",
    ">",
    ">=",
    "==",
    "!=",
    "&",
    "^",
    "|",
    "&&",
    "||",
    "?:",
    "=",
    ",",
    "while",
    "default",
    "case",
    "switch",
    "switch{}",
    "return",
    "if",
    "goto",
    "for",
    "for()",
    "do while",
    "break",
    "{}",
    "arg",
    "arg[]",
    "func proto",
    "func{}",
    "func",
    "var proto",
    "var{}",
    "var",
    "program"
};

struct ast* ast_new() {
    struct ast* ast = malloc(sizeof(struct ast));
    memset(ast, 0, sizeof(struct ast));
    return ast;
}

void ast_free(struct ast* ast) {
    if(ast == NULL) return;
    switch(ast->type) {
        case AST_NUMBER: break;
        case AST_IDENT:
            free(ast->ident.value);
            break;
        case AST_UNARY:
            ast_free(ast->unary.next);
            break;
        case AST_BINARY:
            ast_free(ast->binary.left);
            ast_free(ast->binary.right);
            break;
        case AST_TERNARY:
            ast_free(ast->ternary.left);
            ast_free(ast->ternary.center);
            ast_free(ast->ternary.right);
            break;
        case AST_LIST:
            for(size_t i = 0; i < ast->list.sz; i++) {
                ast_free(ast->list.list[i]);
            }
            break;
    }

    free(ast);
}

void _ast_print(struct ast* ast, int indent) {
    if(ast == NULL) return;
    switch(ast->type) {
        case AST_NUMBER:
            printf("$%d", ast->number.value);
            break;
        case AST_IDENT:
            printf("%s", ast->ident.value);
            break;
        case AST_UNARY:
            if(ast->unary.op >= 0 && ast->unary.op < _OP_ENDOFLIST)
                printf("(%s\n", op_strings[ast->binary.op]);
            else
                printf("(\n");

            for(int i = 0; i < indent; i++) printf(" ");
            _ast_print(ast->unary.next, indent + 4);
            printf("\n");

            for(int i = 0; i < indent - 4; i++) printf(" ");
            printf(")");
            break;
        case AST_BINARY:
            if(ast->binary.op >= 0 && ast->binary.op < _OP_ENDOFLIST)
                printf("(%s\n", op_strings[ast->binary.op]);
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
        case AST_TERNARY:
            if(ast->ternary.op >= 0 && ast->ternary.op < _OP_ENDOFLIST)
                printf("(%s\n", op_strings[ast->ternary.op]);
            else
                printf("(\n");

            for(int i = 0; i < indent; i++) printf(" ");
            _ast_print(ast->ternary.left, indent + 4);
            printf("\n");

            for(int i = 0; i < indent; i++) printf(" ");
            _ast_print(ast->ternary.center, indent + 4);
            printf("\n");

            for(int i = 0; i < indent; i++) printf(" ");
            _ast_print(ast->ternary.right,  indent + 4);
            printf("\n");

            for(int i = 0; i < indent - 4; i++) printf(" ");
            printf(")");
            break;
        case AST_LIST:
            if(ast->list.op >= 0 && ast->list.op < _OP_ENDOFLIST)
                printf("(%s\n", op_strings[ast->list.op]);
            else
                printf("(\n");

            for(size_t idx = 0; idx < ast->list.sz; idx++) {
                for(int i = 0; i < indent; i++) printf(" ");
                _ast_print(ast->list.list[idx], indent + 4);
                printf("\n");
            }

            for(int i = 0; i < indent - 4; i++) printf(" ");
            printf(")");
            break;
    }
}

void ast_print(struct ast* ast) {
    _ast_print(ast, 4);
    printf("\n");
}