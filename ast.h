#ifndef AST_H
#define AST_H

typedef struct AST AST;

/* @ADDNEW */
struct AST {
  enum {
    AST_NUMBER,
    AST_ADD,
    AST_MUL,
  } tag;
  union {
    struct AST_NUMBER { int number; } AST_NUMBER;
    struct AST_ADD { AST *left; AST *right; } AST_ADD;
    struct AST_MUL { AST *left; AST *right; } AST_MUL;
  } data;
};

#define AST_NEW(tag, ...) \
    ast_new((AST){tag, {.tag=(struct tag){__VA_ARGS__}}})

AST* ast_new(AST ast);

#endif