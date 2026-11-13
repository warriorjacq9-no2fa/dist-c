#include "ast.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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
    size_t k_sz = sizeof(struct array) + (dest->k->sz + src->k->sz) * sizeof(void*);
    size_t v_sz = sizeof(struct array) + (dest->v->sz + src->v->sz) * sizeof(void*);

    dest->k = realloc(dest->k, k_sz);
    dest->v = realloc(dest->v, v_sz);

    memcpy(&dest->k->data[dest->k->sz], src->k->data, src->k->sz * sizeof(void*));
    dest->k->sz += src->k->sz;

    memcpy(&dest->v->data[dest->v->sz], src->v->data, src->v->sz * sizeof(void*));
    dest->v->sz += src->v->sz;

    free(src->k);
    free(src->v);
    free(src);
}

void dict_print(struct dict* dict) {
    size_t k_sz = dict->k->sz;
    size_t v_sz = dict->v->sz;
    size_t sz = (k_sz < v_sz) ? k_sz : v_sz;

    for(size_t i = 0; i < sz; i++) {
        printf("%s: %s\n", (char*)dict->k->data[i], (char*)dict->v->data[i]);
    }
}

// TODO: fix mem leak when dict size is 0
/* Returns dict with keys as identifiers and values as (stringified) types */
struct dict* get_idents(struct ast* ast) {
    struct dict* dict = dict_new(0);
    if(ast == NULL) return dict;
    switch(ast->type) {
        case AST_UNARY: {
            struct dict* a = get_idents(ast->unary.next);
            dict_append(dict, a);
            break;
        }
        case AST_BINARY: {
            switch(ast->binary.op) {
                case OP_VAR_PROTO:
                    dict->k = realloc(dict->k, sizeof(struct array) + sizeof(void*));
                    dict->k->sz = 1;
                    dict->k->data[0] = strdup(ast->binary.right->ident.value);
                    dict->v = realloc(dict->v, sizeof(struct array) + sizeof(void*));
                    dict->v->sz = 1;
                    dict->v->data[0] = strdup(ast->binary.left->unary.next->ident.value);
                    break;
                case OP_FUNC_PROTO:
                    dict->k = realloc(dict->k, sizeof(struct array) + sizeof(void*));
                    dict->k->sz = 1;
                    dict->k->data[0] = strdup(ast->binary.right->ident.value);

                    char* type = strdup(ast->binary.left->unary.next->ident.value);
                    size_t sz = strlen(type) + 5;
                    type = realloc(type, sz);
                    memmove(&type[5], type, sz - 5);
                    type[0] = 'f',
                    type[1] = 'u',
                    type[2] = 'n',
                    type[3] = 'c';
                    type[4] = ':';
                    dict->v = realloc(dict->v, sizeof(struct array) + sizeof(void*));
                    dict->v->sz = 1;
                    dict->v->data[0] = type;
                    break;
                default: {
                    struct dict* a = get_idents(ast->binary.left);
                    if(a->k->sz > 0) dict_append(dict, a);

                    struct dict* b = get_idents(ast->binary.right);
                    if(b->k->sz > 0) dict_append(dict, b);
                    break;
                }
            }
            break;
        }
        case AST_TERNARY: {
            struct dict* a = get_idents(ast->ternary.left);
            if(a->k->sz > 0) dict_append(dict, a);

            struct dict* b = get_idents(ast->ternary.center);
            if(b->k->sz > 0) dict_append(dict, b);

            struct dict* c = get_idents(ast->ternary.right);
            if(c->k->sz > 0) dict_append(dict, c);
            break;
        }
        case AST_LIST: {
            for(size_t i = 0; i < ast->list.sz; i++) {
                struct dict* a = get_idents(ast->list.list[i]);
                if(a->k->sz > 0) dict_append(dict, a);
            }
            break;
        }
        default: break;
    }
    return dict;
}

char* compile(struct ast* ast) {
    dict_print(get_idents(ast));
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
    return NULL;
}