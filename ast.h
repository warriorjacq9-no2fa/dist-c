#ifndef AST_H
#define AST_H

#include <stdbool.h>

typedef struct AST AST;

extern char* ast_names[];

struct AST {
    enum ast_token {
        AST_NONE,
        AST_MUL,
        AST_DIV,
        AST_SUM,
        AST_DIF,
        AST_NUMBER,
        _AST_EOL
    } token;
    union {
        int AST_NUMBER;
    } data;
    AST *left; AST *right;
    int line, col;
};

void ast_free(AST* ast);
void ast_print(AST* ast);
bool ast_is_leaf(AST* ast);

#endif