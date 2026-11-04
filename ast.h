#ifndef AST_H
#define AST_H

typedef struct AST AST;

extern char* ast_names[];

struct AST {
    enum ast_token {
        AST_NONE,
        AST_NUMBER,
        AST_SUM,
        AST_MUL
    } token_enum;
    AST *left; AST *right;
    double bp_left, bp_right;
    char* token;
};

void ast_free(AST* ast);
void ast_print(AST* ast);

#endif