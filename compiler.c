#include "ast.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "dict.h"

struct scoped_type {
    struct ast* scope_top;
    char* type;
};

/* Returns dict with keys as identifiers and values as struct scoped_type* */
struct dict* get_idents(struct ast* ast) {
    /* Push on entering new scope */
    static struct ast* stack[1024];
    static size_t sp = 1024;
    struct dict* dict = dict_new(0);
    if(ast == NULL || ast->type == AST_NUMBER || ast->type == AST_IDENT)
        return dict;
    bool is_scope = false;
    switch(ast->unary.op) {
        case OP_ARG:
        case OP_VAR_PROTO: {
            dict->k = realloc(dict->k, sizeof(struct array) + sizeof(void*));
            dict->k->sz = 1;
            dict->k->data[0] = strdup(ast->binary.right->ident.value);

            dict->v = realloc(dict->v, sizeof(struct array) + sizeof(void*));
            dict->v->sz = 1;

            struct scoped_type* t = malloc(sizeof(struct scoped_type));
            t->scope_top = stack[sp];
            t->type = strdup(ast->binary.left->unary.next->ident.value);
            dict->v->data[0] = t;
            break;
        }
        case OP_FUNC_PROTO: {
            dict->k = realloc(dict->k, sizeof(struct array) + sizeof(void*));
            dict->k->sz = 1;
            dict->k->data[0] = strdup(ast->ternary.center->ident.value);

            dict->v = realloc(dict->v, sizeof(struct array) + sizeof(void*));
            dict->v->sz = 1;

            struct scoped_type* t = malloc(sizeof(struct scoped_type));
            char* type = strdup(ast->ternary.left->unary.next->ident.value);
            size_t sz = strlen(type) + 5;
            type = realloc(type, sz);
            memmove(&type[5], type, sz - 5);
            type[0] = 'f',
            type[1] = 'u',
            type[2] = 'n',
            type[3] = 'c';
            type[4] = ':';

            t->scope_top = stack[sp];
            t->type = type;
            dict->v->data[0] = t;

            goto _case_ternary;
            break;
        }
        case OP_SWITCH_BODY:
        case OP_STMT_BODY:
        case OP_FUNC:
        case OP_PROGRAM:
            is_scope = true;
            [[fallthrough]];
        default:
            if(is_scope) stack[--sp] = ast;
            switch(ast->type) {
                case AST_UNARY: {
                    struct dict* a = get_idents(ast->unary.next);
                    dict_append(dict, a);
                    break;
                }
                case AST_BINARY: {
                    struct dict* a = get_idents(ast->binary.left);
                    if(a->k->sz > 0) dict_append(dict, a);
                    else dict_free(a);

                    struct dict* b = get_idents(ast->binary.right);
                    if(b->k->sz > 0) dict_append(dict, b);
                    else dict_free(b);
                    break;
                }
                case AST_TERNARY:
_case_ternary: {
                    struct dict* a = get_idents(ast->ternary.left);
                    if(a->k->sz > 0) dict_append(dict, a);
                    else dict_free(a);

                    struct dict* b = get_idents(ast->ternary.center);
                    if(b->k->sz > 0) dict_append(dict, b);
                    else dict_free(b);

                    struct dict* c = get_idents(ast->ternary.right);
                    if(c->k->sz > 0) dict_append(dict, c);
                    else dict_free(c);
                    break;
                }
                case AST_LIST: {
                    for(size_t i = 0; i < ast->list.sz; i++) {
                        struct dict* a = get_idents(ast->list.list[i]);
                        if(a->k->sz > 0) dict_append(dict, a);
                        else dict_free(a);
                    }
                    break;
                }
                default: break;
            }
            if(is_scope) sp++;
            break;
    }
    return dict;
}

char* compile(struct ast* ast) {
    struct dict* dict = get_idents(ast);

    size_t k_sz = dict->k->sz;
    size_t v_sz = dict->v->sz;
    size_t sz = (k_sz < v_sz) ? k_sz : v_sz;

    printf("{\n");
    for(size_t i = 0; i < sz; i++) {
        struct scoped_type* t = dict->v->data[i];
        printf("    %s: %s %p(%s)\n",
            (char*)dict->k->data[i], t->type, t->scope_top,
            op_strings[t->scope_top->unary.op]
        );
    }
    printf("}\n");
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