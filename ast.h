#ifndef AST_H
#define AST_H

#include <stddef.h>
#include "lexer.h"

extern const char* const op_strings[];

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
/* parse_if */
    OP_IF,
/* parse_goto */
    OP_GOTO,
/* parse_for */
    OP_FOR,
    OP_FOR_CLAUSE,
/* do_while */
    OP_DO_WHILE,
/* parse_break */
    OP_BREAK,
/* stmt */
    OP_STMT_BODY,
/* arg */
    OP_ARG,
/* arg_list */
    OP_ARG_LIST,
/* decl */
    OP_FUNC_PROTO,
    OP_FUNC_BODY,
    OP_FUNC,
    OP_VAR_PROTO,
    OP_VAR_BODY,
    OP_VAR,
/* program */
    OP_PROGRAM,

    _OP_ENDOFLIST
};

struct ast {
    union {
        struct { int value; } number;
        struct { char* value; } ident;
        struct { enum op op; struct ast* next; } unary;
        struct { enum op op; struct ast* left; struct ast* right; } binary;
        struct { enum op op; struct ast* left; struct ast* center; struct ast* right; } ternary;
        struct { enum op op; struct ast** list; size_t sz; } list;
    };
    enum ast_type {
        AST_NUMBER,
        AST_IDENT,
        AST_UNARY,
        AST_BINARY,
        AST_TERNARY,
        AST_LIST
    } type;
};

struct ast* ast_new();
void ast_free(struct ast* ast);
void ast_print(struct ast* ast);

#endif