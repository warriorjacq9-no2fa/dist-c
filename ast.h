#ifndef AST_H
#define AST_H

#include <stddef.h>
#include "lexer.h"

struct ast {
    union {
        struct { int value; } number;
        struct { char* value; } ident;
        struct { enum token tok; struct ast* left; struct ast* right; } binary;
    };
    enum ast_type {
        AST_NUMBER,
        AST_IDENT,
        AST_BINARY
    } type;
};

struct ast* ast_new();

#endif