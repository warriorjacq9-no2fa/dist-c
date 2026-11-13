#include "ast.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

struct array {
    size_t sz;
    void* data[];
};

struct dict {
    struct array* k;
    struct array* v;
};

struct dict* dict_new(size_t sz) {
    struct dict* dict = malloc(sizeof(struct dict));
    dict->k = malloc(sizeof(struct array) + sz * sizeof(void*));
    dict->k->sz = sz;
    dict->v = malloc(sizeof(struct array) + sz * sizeof(void*));
    dict->v->sz = sz;
    return dict;
}

/* Append src onto dict and destroy src */
void dict_append(struct dict* dest, struct dict* src) {
    dest->k = realloc(dest->k, dest->k->sz + src->k->sz);
    memcpy(&dest->k[dest->k->sz], src->k, src->k->sz);
    dest->k->sz += src->k->sz;
    for(size_t i = 0; i < src->k->sz; i++) free(src->k->data[i]);
    free(src->k);

    dest->v = realloc(dest->v, dest->v->sz + src->v->sz);
    memcpy(&dest->v[dest->v->sz], src->v, src->v->sz);
    dest->v->sz += src->v->sz;
    for(size_t i = 0; i < src->v->sz; i++) free(src->v->data[i]);
    free(src->v);
}

/* Returns dict with keys as identifiers and values as (stringified) types */
struct dict* get_idents(struct ast* ast) {
    struct dict* dict = dict_new(1);
    switch(ast->type) {
        case AST_BINARY: {
            switch(ast->binary.op) {
                case OP_VAR_PROTO:
                    dict->k->data[0] = ast->binary.right->ident.value;
                    dict->v->data[0] = ast->binary.left->unary.next->ident.value;
                    break;
                default: {
                    struct dict* a = get_idents(ast->binary.left);
                    dict_append(dict, a);

                    struct dict* b = get_idents(ast->binary.right);
                    dict_append(dict, a);
                    break;
                }
            }
            break;
        }
    }
}

char* compile(struct ast* ast) {
    switch(ast->unary.op) {
        case OP_TYPE:
        case OP_POST:
        case OP_POST_INC:
        case OP_POST_DEC:
        case OP_CALL:
        case OP_CALL_ARGS:
        case OP_ARRAY_DEREF:
        case OP_PRE_INC:
        case OP_PRE_DEC:
        case OP_UNARY_PLUS:
        case OP_UNARY_MINUS:
        case OP_BOOLINV:
        case OP_LOGINV:
        case OP_TYPECAST:
        case OP_DEREF:
        case OP_REF:
        case OP_MUL:
        case OP_DIV:
        case OP_MOD:
        case OP_ADD:
        case OP_SUB:
        case OP_LSHIFT:
        case OP_RSHIFT:
        case OP_LESS:
        case OP_LEQ:
        case OP_GREATER:
        case OP_GEQ:
        case OP_EQUALTO:
        case OP_NEQ:
        case OP_AND:
        case OP_XOR:
        case OP_OR:
        case OP_BOOLAND:
        case OP_BOOLOR:
        case OP_TERNARY:
        case OP_ASSIGN:
        case OP_COMMA:
        case OP_WHILE:
        case OP_DEFAULT:
        case OP_CASE:
        case OP_SWITCH:
        case OP_SWITCH_BODY:
        case OP_RETURN:
        case OP_IF:
        case OP_GOTO:
        case OP_FOR:
        case OP_FOR_CLAUSE:
        case OP_DO_WHILE:
        case OP_BREAK:
        case OP_STMT_BODY:
        case OP_ARG:
        case OP_ARG_LIST:
        case OP_FUNC_PROTO:
        case OP_FUNC_BODY:
        case OP_FUNC:
        case OP_VAR_PROTO:
        case OP_VAR_BODY:
        case OP_VAR:
        case OP_PROGRAM:
        default: break;
    }
}