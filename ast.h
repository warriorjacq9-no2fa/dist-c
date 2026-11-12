#ifndef AST_H
#define AST_H

#include <stddef.h>
#include "lexer.h"

enum op {
/* type */
    OP_TYPE,
/* expr_1 */
    OP_POST,
    OP_POST_INC,
    OP_POST_DEC,
    OP_CALL,
    OP_CALL_ARGS,
    OP_ARRAY_DEREF,
/* expr_2 */
    OP_PRE_INC,
    OP_PRE_DEC,
    OP_UNARY_PLUS,
    OP_UNARY_MINUS,
    OP_BOOLINV,
    OP_LOGINV,
    OP_TYPECAST,
    OP_DEREF,
    OP_REF,
/* expr_3 */
    OP_MUL,
    OP_DIV,
    OP_MOD,
/* expr_4 */
    OP_ADD,
    OP_SUB,
/* expr_5 */
    OP_LSHIFT,
    OP_RSHIFT,
/* expr_6 */
    OP_LESS,
    OP_LEQ,
    OP_GREATER,
    OP_GEQ,
/* expr_7 */
    OP_EQUALTO,
    OP_NEQ,
/* expr_8 */
    OP_AND,
/* expr_9 */
    OP_XOR,
/* expr_10 */
    OP_OR,
/* expr_11 */
    OP_BOOLAND,
/* expr_12 */
    OP_BOOLOR,
/* expr_13 */
    OP_TERNARY,
    OP_TERNARY_RES,
/* expr_14 */
    OP_ASSIGN,
/* expr_15 */
    OP_COMMA,
/* parse_while */
    OP_WHILE,
/* parse_default */
    OP_DEFAULT,
/* parse_case */
    OP_CASE,
/* parse_switch */
    OP_SWITCH,
    OP_SWITCH_BODY,
/* parse_return */
    OP_RETURN,
/* stmt */
    OP_STMT_BODY,

    _OP_ENDOFLIST
};

struct ast {
    union {
        struct { int value; } number;
        struct { char* value; } ident;
        struct { enum op op; struct ast* next; } unary;
        struct { enum op op; struct ast* left; struct ast* right; } binary;
        struct { enum op op; struct ast** list; size_t sz; } list;
    };
    enum ast_type {
        AST_NUMBER,
        AST_IDENT,
        AST_UNARY,
        AST_BINARY,
        AST_LIST
    } type;
};

struct ast* ast_new();
void ast_free(struct ast* ast);
void ast_print(struct ast* ast);

#endif