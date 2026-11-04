#ifndef AST_H
#define AST_H

#include <stdbool.h>

typedef struct AST AST;

extern char* ast_names[];

struct AST {
    enum ast_token {
        AST_NONE,
        AST_NUMBER,
        AST_SUM,
        AST_MUL
    } token;
    union {
        int AST_NUMBER;
    } data;
    AST *left; AST *right;
};

void ast_free(AST* ast);
void ast_print(AST* ast);
bool ast_is_leaf(AST* ast);

#endif