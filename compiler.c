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

void emit(char* s) {
    fprintf(stderr, "%s", s);
}

/* Returns dict with keys as identifiers and values as struct scoped_type* */
struct dict* get_idents(struct ast* ast) {
    /* Push on entering new scope */
    static struct ast* stack[128];
    static size_t sp = 128;
    struct dict* dict = dict_new(0);
    if(ast == NULL || ast->type == AST_NUMBER || ast->type == AST_IDENT)
        return dict;
    bool is_scope = false;
    if(ast->type == AST_NUMBER || ast->type == AST_IDENT) goto _default;
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
            struct ast* id = ast->ternary.left;
            dict->k = realloc(dict->k, sizeof(struct array) + sizeof(void*));
            dict->k->sz = 1;
            dict->k->data[0] = strdup(id->binary.right->ident.value);

            dict->v = realloc(dict->v, sizeof(struct array) + sizeof(void*));
            dict->v->sz = 1;

            struct scoped_type* t = malloc(sizeof(struct scoped_type));
            char* type = strdup(id->binary.left->unary.next->ident.value);
            size_t sz = strlen(type) + 5;
            type = realloc(type, sz);
            memmove(&type[2], type, sz - 2);
            type[0] = 'f',
            type[1] = ':';

            t->scope_top = stack[sp];
            t->type = type;
            dict->v->data[0] = t;

            if(ast->ternary.right) {
                stack[--sp] = ast->ternary.right;
            } else {
                stack[--sp] = ast;
            }
            is_scope = true;
            goto _case_ternary;
            break;
        }
        case OP_FOR:
        case OP_SWITCH_BODY:
        case OP_STMT_BODY:
        case OP_FUNC_BODY:
        case OP_PROGRAM:
            is_scope = true;
            [[fallthrough]];
        default: _default:
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

void do_compile(struct ast* ast) {
    if(ast == NULL) return;
    switch(ast->type) {
        case AST_NUMBER: {
            char buf[256];
            snprintf(buf, 256, "mov eax, %d\n", ast->number.value);
            emit(buf);
            break;
        }
        case AST_IDENT: {
            char buf[256];
            snprintf(buf, 256, "mov eax, [%s]\n", ast->ident.value);
            emit(buf);
            break;
        }
        default:
            switch(ast->unary.op) {
                case OP_POST_INC:
                    do_compile(ast->unary.next);
                    emit("inc eax\n");
                    break;
                case OP_POST_DEC:
                    do_compile(ast->unary.next);
                    emit("dec eax\n");
                    break;
                case OP_CALL: {
                    struct ast* args = ast->binary.right;
                    for(int i = args->list.sz - 1; i >= 0; i--) {
                        do_compile(args->list.list[i]);
                        emit("push eax\n");
                    }
                    do_compile(ast->binary.left);
                    emit("call eax\n");
                    break;
                }
                case OP_ARRAY_DEREF:
                    do_compile(ast->binary.left);
                    emit("push eax\n");
                    do_compile(ast->binary.right);
                    emit("mov ecx, eax\n");
                    emit("pop ebx\n");

                    emit("mov eax, [ebx + ecx]\n");
                    break;
                case OP_PRE_INC:
                    emit("inc eax\n");
                    do_compile(ast->unary.next);
                    break;
                case OP_PRE_DEC:
                    emit("dec eax\n");
                    do_compile(ast->unary.next);
                    break;
                case OP_UNARY_PLUS:
                    break;
                case OP_UNARY_MINUS:
                    emit("neg eax\n");
                    do_compile(ast->unary.next);
                    break;
                case OP_BOOLINV:
                    emit("test eax, eax\n");
                    emit("setz al");
                    emit("movzx eax, al\n");
                    do_compile(ast->unary.next);
                    break;
                case OP_LOGINV:
                    emit("not eax\n");
                    do_compile(ast->unary.next);
                    break;
                case OP_DEREF:
                    emit("mov eax, [eax]\n");
                    break;
                case OP_REF:
                    break;
                case OP_MUL:
                    do_compile(ast->binary.left);
                    emit("push eax\n");
                    do_compile(ast->binary.right);
                    emit("pop ecx\n");
                    emit("mul ecx\n");
                    break;
                case OP_DIV:
                    do_compile(ast->binary.left);
                    emit("push eax\n");
                    do_compile(ast->binary.right);
                    emit("mov ecx, eax\n");
                    emit("xor edx, edx\n");
                    emit("pop eax\n");
                    emit("div ecx\n");
                    break;
                case OP_MOD:
                    do_compile(ast->binary.left);
                    emit("push eax\n");
                    do_compile(ast->binary.right);
                    emit("mov ecx, eax\n");
                    emit("xor edx, edx\n");
                    emit("pop eax\n");
                    emit("div ecx\n");
                    emit("mov eax, edx\n");
                    break;
                case OP_ADD:
                    do_compile(ast->binary.left);
                    emit("push eax\n");
                    do_compile(ast->binary.right);
                    emit("pop ecx\n");
                    emit("add eax, ecx\n");
                    break;
                case OP_SUB:
                    do_compile(ast->binary.left);
                    emit("mov ecx, eax\n");
                    do_compile(ast->binary.right);
                    emit("xchg ecx, eax\n");
                    emit("sub eax, ecx\n");
                    break;
                case OP_LSHIFT:
                    do_compile(ast->binary.left);
                    emit("mov ecx, eax\n");
                    do_compile(ast->binary.right);
                    emit("xchg ecx, eax\n");
                    emit("shl eax, cl\n");
                    break;
                case OP_RSHIFT:
                    do_compile(ast->binary.left);
                    emit("mov ecx, eax\n");
                    do_compile(ast->binary.right);
                    emit("xchg ecx, eax\n");
                    emit("shr eax, cl\n");
                    break;
                case OP_LESS:
                    do_compile(ast->binary.left);
                    emit("mov ecx, eax\n");
                    do_compile(ast->binary.right);
                    emit("test ecx, eax\n");
                    emit("setb al\n");
                    emit("movzx eax, al\n");
                    break;
                case OP_LEQ:
                    do_compile(ast->binary.left);
                    emit("mov ecx, eax\n");
                    do_compile(ast->binary.right);
                    emit("test ecx, eax\n");
                    emit("setbe al\n");
                    emit("movzx eax, al");
                    break;
                case OP_GREATER:
                    do_compile(ast->binary.left);
                    emit("mov ecx, eax\n");
                    do_compile(ast->binary.right);
                    emit("test ecx, eax\n");
                    emit("seta al\n");
                    emit("movzx eax, al\n");
                    break;
                case OP_GEQ:
                    do_compile(ast->binary.left);
                    emit("mov ecx, eax\n");
                    do_compile(ast->binary.right);
                    emit("test ecx, eax\n");
                    emit("setae al\n");
                    emit("movzx eax, al\n");
                    break;
                case OP_EQUALTO:
                    do_compile(ast->binary.left);
                    emit("mov ecx, eax\n");
                    do_compile(ast->binary.right);
                    emit("test ecx, eax\n");
                    emit("sete al\n");
                    emit("movzx eax, al\n");
                    break;
                case OP_NEQ:
                    do_compile(ast->binary.left);
                    emit("mov ecx, eax\n");
                    do_compile(ast->binary.right);
                    emit("test ecx, eax\n");
                    emit("setne al\n");
                    emit("movzx eax, al\n");
                    break;
                case OP_AND:
                    do_compile(ast->binary.left);
                    emit("mov ecx, eax\n");
                    do_compile(ast->binary.right);
                    emit("and eax, ecx\n");
                    break;
                case OP_XOR:
                    do_compile(ast->binary.left);
                    emit("mov ecx, eax\n");
                    do_compile(ast->binary.right);
                    emit("xor eax, ecx\n");
                    break;
                case OP_OR:
                    do_compile(ast->binary.left);
                    emit("mov ecx, eax\n");
                    do_compile(ast->binary.right);
                    emit("or eax, ecx\n");
                    break;
                case OP_BOOLAND:
                    do_compile(ast->binary.left);
                    emit("test eax, eax\n");
                    emit("setnz bl\n");
                    do_compile(ast->binary.right);
                    emit("test eax, eax\n");
                    emit("setnz al\n");
                    emit("and al, bl\n");
                    emit("movzx eax, al\n");
                    break;
                case OP_BOOLOR:
                    do_compile(ast->binary.left);
                    emit("test eax, eax\n");
                    emit("setnz bl\n");
                    do_compile(ast->binary.right);
                    emit("test eax, eax\n");
                    emit("setnz al\n");
                    emit("or al, bl\n");
                    emit("movzx eax, al\n");
                    break;
                case OP_TERNARY:
                    break; // TODO: ternary
                case OP_ASSIGN:
                    do_compile(ast->binary.left);
                    emit("mov ebx, eax\n");
                    do_compile(ast->binary.right);
                    emit("mov [ebx], eax\n");
                    break; // TODO: fix assign
                case OP_COMMA:
                    do_compile(ast->binary.left);
                    do_compile(ast->binary.right);
                    break;

                case OP_WHILE: {
                    char buf[256];
                    snprintf(buf, 256, "_while_%p:\n", ast);
                    emit(buf);

                    do_compile(ast->binary.left);

                    emit("test eax, eax\n");
                    snprintf(buf, 256, "jz _end_while_%p\n", ast);
                    emit(buf);

                    do_compile(ast->binary.right);

                    snprintf(buf, 256, "jmp _while_%p\n", ast);
                    emit(buf);

                    snprintf(buf, 256, "_end_while_%p:\n", ast);
                    emit(buf);
                    break;
                }
                // TODO: switches
                case OP_DEFAULT:
                case OP_CASE:
                case OP_SWITCH:
                case OP_SWITCH_BODY:
                case OP_RETURN:
                    do_compile(ast->unary.next);
                    emit("ret\n");
                    break;
                // TODO: other conditionals
                case OP_IF: {
                    do_compile(ast->ternary.left);
                    emit("test eax, eax\n");
                    char buf[256];
                    snprintf(buf, 256, "jz _else_%p\n", ast);
                    emit(buf);
                    do_compile(ast->ternary.center);
                    snprintf(buf, 256, "_else_%p:\n", ast);
                    do_compile(ast->ternary.right);
                    break;
                }
                case OP_GOTO: {
                    char buf[256];
                    snprintf(buf, 256, "jmp %s\n", ast->unary.next->ident.value);
                    emit(buf);
                    break;
                }
                case OP_FOR: {
                    struct ast* clause = ast->binary.left;
                    
                    do_compile(clause->ternary.left);

                    char buf[256];
                    snprintf(buf, 256, "_for_%p:\n", ast);
                    emit(buf);

                    do_compile(clause->ternary.center);

                    emit("test eax, eax\n");
                    snprintf(buf, 256, "jz _end_for_%p\n", ast);
                    emit(buf);

                    do_compile(ast->binary.right);
                    do_compile(clause->ternary.right);

                    snprintf(buf, 256, "jmp _for_%p\n", ast);
                    emit(buf);

                    snprintf(buf, 256, "_end_for_%p:\n", ast);
                    emit(buf);
                    break;
                }
                case OP_DO_WHILE: {
                    char buf[256];
                    snprintf(buf, 256, "_dowhile_%p:\n", ast);
                    emit(buf);
                    do_compile(ast->binary.right);
                    do_compile(ast->binary.left);
                    emit("test eax, eax\n");
                    snprintf(buf, 256, "jnz _dowhile_%p\n", ast);
                    emit(buf);
                    break;
                }
                case OP_BREAK:
                    break;

                case OP_STMT_BODY:
                case OP_FUNC_BODY:
                case OP_PROGRAM:
                    for(size_t i = 0; i < ast->list.sz; i++)
                        do_compile(ast->list.list[i]);
                    break;

                case OP_FUNC_PROTO: {
                    struct ast* body = ast->ternary.right;
                    if(body) {
                        for(size_t i = 0; i < body->list.sz; i++)
                        do_compile(body->list.list[i]);
                    }
                    break;
                }
                
                default:
                    break;
            }
    }
}

void compile(struct ast* ast) {
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
    do_compile(ast);
}