#include "ast.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "dict.h"

#define BUFFER_SZ 1024

enum value {
    VAL_NONE,
    VAL_LITERAL,
    VAL_PTR
};

struct scoped_type {
    struct ast* scope_top;
    char* type;
    char* replacement;
    enum value replacement_type;
};

static struct dict* vars;
static char buffer[BUFFER_SZ];
static size_t buf_idx = 0;

void emit(char* s) {
    snprintf(&buffer[buf_idx], BUFFER_SZ - buf_idx, "%s", s);
    buf_idx += strlen(s);
}

[[noreturn]] void c_error(struct ast* ast, char* msg) { // TODO: line numbers
    fprintf(stderr, "%s\n", msg);
    exit(1);
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
// TODO: types
enum value do_compile(struct ast* ast) {
    if(ast == NULL) return VAL_NONE;
    switch(ast->type) {
        case AST_NUMBER: {
            char buf[256];
            snprintf(buf, 256, "    mov rax, %d\n", ast->number.value);
            emit(buf);
            return VAL_LITERAL;
        }
        case AST_IDENT: {
            char buf[256];
            struct scoped_type* t = dict_get(vars, ast->ident.value);
            if(t && t->replacement) {
                snprintf(buf, 256, "    mov rax, %s\n", t->replacement);
                emit(buf);
                return t->replacement_type;
            } else {
                snprintf(buf, 256, 
                    "Symbol %s not defined", ast->ident.value
                );
                c_error(ast, buf);
            }
        }
        default:
            switch(ast->unary.op) {
                case OP_POST_INC: {
                    enum value v = do_compile(ast->unary.next);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    inc rax\n");
                            break;
                        case VAL_PTR:
                            emit("    inc [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected lvalue");
                            break;
                    }
                    break;
                }
                case OP_POST_DEC: {
                    enum value v = do_compile(ast->unary.next);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    dec rax\n");
                            break;
                        case VAL_PTR:
                            emit("    dec [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected lvalue");
                            break;
                    }
                    break;
                }
                case OP_CALL: { // TODO: fix CALL
                    struct ast* args = ast->binary.right;
                    for(int i = args->list.sz - 1; i >= 0; i--) {
                        enum value v = do_compile(args->list.list[i]);
                        switch(v) {
                            case VAL_LITERAL:
                                emit("    push rax\n");
                                break;
                            case VAL_PTR:
                                emit("    push [rax]\n");
                                break;
                            default:
                                c_error(ast, "Expected rvalue");
                                break;
                        }
                    }
                    do_compile(ast->binary.left);
                    emit("    call rax\n");
                    for(int i = 0; i < args->list.sz; i++) {
                        emit("    add rsp, 8\n");
                    }
                    break;
                }
                case OP_ARRAY_DEREF: {
                    enum value v = do_compile(ast->binary.left);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    push rax\n");
                            break;
                        case VAL_PTR:
                            emit("    push [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected lvalue");
                            break;
                    }
                    v = do_compile(ast->binary.right);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    mov rcx, rax\n");
                            break;
                        case VAL_PTR:
                            emit("    mov rcx, [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected rvalue");
                            break;
                    }
                    emit("    pop rbx\n");

                    emit("    mov rax, [rbx + rcx]\n");
                    break;
                }
                case OP_PRE_INC: { // TODO: proper pre inc and dec
                    enum value v = do_compile(ast->unary.next);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    inc rax\n");
                            break;
                        case VAL_PTR:
                            emit("    inc [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected lvalue");
                            break;
                    }
                    break;
                }
                case OP_PRE_DEC: {
                    enum value v = do_compile(ast->unary.next);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    dec rax\n");
                            break;
                        case VAL_PTR:
                            emit("    dec [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected lvalue");
                            break;
                    }
                    break;
                }
                case OP_UNARY_PLUS:
                    break;
                case OP_UNARY_MINUS: {
                    enum value v = do_compile(ast->unary.next);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    neg rax\n");
                            break;
                        case VAL_PTR:
                            emit("    neg [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected lvalue");
                            break;
                    }
                    break;
                }
                case OP_BOOLINV: {
                    enum value v = do_compile(ast->unary.next);
                    if(v == VAL_PTR) emit("    mov rax, [rax]\n");
                    emit("    test rax, rax\n");
                    emit("    setz al\n");
                    emit("    movzx rax, al\n");
                    break;
                }
                case OP_LOGINV: {
                    enum value v = do_compile(ast->unary.next);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    not rax\n");
                            break;
                        case VAL_PTR:
                            emit("    not [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected lvalue");
                            break;
                    }
                    break;
                }
                case OP_DEREF: {
                    enum value v = do_compile(ast->unary.next);
                    if(v == VAL_PTR) emit("    mov rax, [rax]\n");
                    emit("    mov rax, [rax]\n");
                    break;
                }
                case OP_REF:
                    break;
                case OP_MUL: {
                    enum value v = do_compile(ast->binary.left);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    push rax\n");
                            break;
                        case VAL_PTR:
                            emit("    push [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected lvalue");
                            break;
                    }
                    v = do_compile(ast->binary.right);
                    emit("    pop rcx\n");
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    mul rcx\n");
                            break;
                        case VAL_PTR:
                            emit("    mul [rcx]\n");
                            break;
                        default:
                            c_error(ast, "Expected rvalue");
                            break;
                    }
                    break;
                }
                case OP_DIV: {
                    enum value v = do_compile(ast->binary.left);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    push rax\n");
                            break;
                        case VAL_PTR:
                            emit("    push [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected lvalue");
                            break;
                    }
                    v = do_compile(ast->binary.right);
                    emit("    mov rcx, rax\n");
                    emit("    xor rdx, rdx\n");
                    emit("    pop rax\n");
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    div rcx\n");
                            break;
                        case VAL_PTR:
                            emit("    div [rcx]\n");
                            break;
                        default:
                            c_error(ast, "Expected rvalue");
                            break;
                    }
                    break;
                }
                case OP_MOD: {
                    enum value v = do_compile(ast->binary.left);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    push rax\n");
                            break;
                        case VAL_PTR:
                            emit("    push [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected lvalue");
                            break;
                    }
                    v = do_compile(ast->binary.right);
                    emit("    mov rcx, rax\n");
                    emit("    xor rdx, rdx\n");
                    emit("    pop rax\n");
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    div rcx\n");
                            break;
                        case VAL_PTR:
                            emit("    div [rcx]\n");
                            break;
                        default:
                            c_error(ast, "Expected rvalue");
                            break;
                    }
                    emit("    mov rax, rdx\n");
                    break;
                }
                case OP_ADD: {
                    enum value v = do_compile(ast->binary.left);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    push rax\n");
                            break;
                        case VAL_PTR:
                            emit("    push [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected lvalue");
                            break;
                    }
                    v = do_compile(ast->binary.right);
                    if(v == VAL_PTR) emit("    mov rax, [rax]\n");
                    emit("    pop rcx\n");
                    emit("    add rax, rcx\n");
                    break;
                }
                case OP_SUB: {
                    enum value v = do_compile(ast->binary.left);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    push rax\n");
                            break;
                        case VAL_PTR:
                            emit("    push [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected lvalue");
                            break;
                    }
                    v = do_compile(ast->binary.right);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    mov rcx, rax\n");
                            break;
                        case VAL_PTR:
                            emit("    mov rcx, [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected rvalue");
                            break;
                    }
                    emit("    pop rax\n");
                    emit("    sub rax, rcx\n");
                    break;
                }
                case OP_LSHIFT: { // TODO: shift by full size number
                    enum value v = do_compile(ast->binary.left);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    push rax\n");
                            break;
                        case VAL_PTR:
                            emit("    push [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected lvalue");
                            break;
                    }
                    v = do_compile(ast->binary.right);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    mov rcx, rax\n");
                            break;
                        case VAL_PTR:
                            emit("    mov rcx, [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected rvalue");
                            break;
                    }
                    emit("    pop rax\n");
                    emit("    shl rax, cl\n");
                    break;
                }
                case OP_RSHIFT: {
                    enum value v = do_compile(ast->binary.left);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    push rax\n");
                            break;
                        case VAL_PTR:
                            emit("    push [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected lvalue");
                            break;
                    }
                    v = do_compile(ast->binary.right);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    mov rcx, rax\n");
                            break;
                        case VAL_PTR:
                            emit("    mov rcx, [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected rvalue");
                            break;
                    }
                    emit("    pop rax\n");
                    emit("    shr rax, cl\n");
                    break;
                }
                case OP_LESS: {
                    enum value v = do_compile(ast->binary.left);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    push rax\n");
                            break;
                        case VAL_PTR:
                            emit("    push [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected lvalue");
                            break;
                    }
                    v = do_compile(ast->binary.right);
                    emit("    pop rcx\n");
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    cmp rcx, rax\n");
                            break;
                        case VAL_PTR:
                            emit("    cmp rcx, [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected rvalue\n");
                            break;
                    }
                    emit("    setb al\n");
                    emit("    movzx rax, al\n");
                    break;
                }
                case OP_LEQ: {
                    enum value v = do_compile(ast->binary.left);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    push rax\n");
                            break;
                        case VAL_PTR:
                            emit("    push [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected lvalue");
                            break;
                    }
                    v = do_compile(ast->binary.right);
                    emit("    pop rcx\n");
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    cmp rcx, rax\n");
                            break;
                        case VAL_PTR:
                            emit("    cmp rcx, [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected rvalue\n");
                            break;
                    }
                    emit("    setbe al\n");
                    emit("    movzx rax, al\n");
                    break;
                }
                case OP_GREATER: {
                    enum value v = do_compile(ast->binary.left);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    push rax\n");
                            break;
                        case VAL_PTR:
                            emit("    push [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected lvalue");
                            break;
                    }
                    v = do_compile(ast->binary.right);
                    emit("    pop rcx\n");
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    cmp rcx, rax\n");
                            break;
                        case VAL_PTR:
                            emit("    cmp rcx, [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected rvalue\n");
                            break;
                    }
                    emit("    seta al\n");
                    emit("    movzx rax, al\n");
                    break;
                }
                case OP_GEQ: {
                    enum value v = do_compile(ast->binary.left);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    push rax\n");
                            break;
                        case VAL_PTR:
                            emit("    push [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected lvalue");
                            break;
                    }
                    v = do_compile(ast->binary.right);
                    emit("    pop rcx\n");
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    cmp rcx, rax\n");
                            break;
                        case VAL_PTR:
                            emit("    cmp rcx, [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected rvalue\n");
                            break;
                    }
                    emit("    setae al\n");
                    emit("    movzx rax, al\n");
                    break;
                }
                case OP_EQUALTO: {
                    enum value v = do_compile(ast->binary.left);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    push rax\n");
                            break;
                        case VAL_PTR:
                            emit("    push [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected lvalue");
                            break;
                    }
                    v = do_compile(ast->binary.right);
                    emit("    pop rcx\n");
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    cmp rcx, rax\n");
                            break;
                        case VAL_PTR:
                            emit("    cmp rcx, [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected rvalue\n");
                            break;
                    }
                    emit("    sete al\n");
                    emit("    movzx rax, al\n");
                    break;
                }
                case OP_NEQ: {
                    enum value v = do_compile(ast->binary.left);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    push rax\n");
                            break;
                        case VAL_PTR:
                            emit("    push [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected lvalue");
                            break;
                    }
                    v = do_compile(ast->binary.right);
                    emit("    pop rcx\n");
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    cmp rcx, rax\n");
                            break;
                        case VAL_PTR:
                            emit("    cmp rcx, [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected rvalue\n");
                            break;
                    }
                    emit("    setne al\n");
                    emit("    movzx rax, al\n");
                    break;
                }
                case OP_AND: {
                    enum value v = do_compile(ast->binary.left);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    push rax\n");
                            break;
                        case VAL_PTR:
                            emit("    push [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected lvalue");
                            break;
                    }
                    v = do_compile(ast->binary.right);
                    if(v == VAL_PTR) emit("    mov rax, [rax]\n");
                    emit("    pop rcx\n");
                    emit("    and rax, rcx\n");
                    break;
                }
                case OP_XOR: {
                    enum value v = do_compile(ast->binary.left);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    push rax\n");
                            break;
                        case VAL_PTR:
                            emit("    push [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected lvalue");
                            break;
                    }
                    v = do_compile(ast->binary.right);
                    if(v == VAL_PTR) emit("    mov rax, [rax]\n");
                    emit("    pop rcx\n");
                    emit("    xor rax, rcx\n");
                    break;
                }
                case OP_OR: {
                    enum value v = do_compile(ast->binary.left);
                    switch(v) {
                        case VAL_LITERAL:
                            emit("    push rax\n");
                            break;
                        case VAL_PTR:
                            emit("    push [rax]\n");
                            break;
                        default:
                            c_error(ast, "Expected lvalue");
                            break;
                    }
                    v = do_compile(ast->binary.right);
                    if(v == VAL_PTR) emit("    mov rax, [rax]\n");
                    emit("    pop rcx\n");
                    emit("    or rax, rcx\n");
                    break;
                }
                case OP_BOOLAND: {
                    enum value v = do_compile(ast->binary.left);
                    if(v == VAL_PTR) emit("    mov rax, [rax]\n");
                    emit("    test rax, rax\n");
                    emit("    setnz al\n");
                    emit("    push rax\n");
                    v = do_compile(ast->binary.right);
                    if(v == VAL_PTR) emit("    mov rax, [rax]\n");
                    emit("    test rax, rax\n");
                    emit("    setnz al\n");
                    emit("    pop rbx\n");
                    emit("    and al, bl\n");
                    emit("    movzx rax, al\n");
                    break;
                }
                case OP_BOOLOR: {
                    enum value v = do_compile(ast->binary.left);
                    if(v == VAL_PTR) emit("    mov rax, [rax]\n");
                    emit("    test rax, rax\n");
                    emit("    setnz al\n");
                    emit("    push rax\n");
                    v = do_compile(ast->binary.right);
                    if(v == VAL_PTR) emit("    mov rax, [rax]\n");
                    emit("    test rax, rax\n");
                    emit("    setnz al\n");
                    emit("    pop bl\n");
                    emit("    or al, bl\n");
                    emit("    movzx rax, al\n");
                    break;
                }
                case OP_TERNARY:
                    break; // TODO: ternary
                case OP_ASSIGN: {
                    enum value v = do_compile(ast->binary.left);
                    if(v != VAL_PTR) c_error(ast, "Invalid lvalue for assign");
                    emit("    push rax\n");
                    v = do_compile(ast->binary.right);
                    emit("    pop rbx\n");
                    if(v == VAL_PTR) emit("    mov rax, [rax]\n");
                    emit("    mov [rbx], rax\n");
                    break;
                }
                case OP_COMMA:
                    do_compile(ast->binary.left);
                    do_compile(ast->binary.right);
                    break;

                case OP_WHILE: {
                    char buf[256];
                    snprintf(buf, 256, "_while_%p:\n", ast);
                    emit(buf);

                    do_compile(ast->binary.left);

                    emit("    test rax, rax\n");
                    snprintf(buf, 256, "    jz _end_while_%p\n", ast);
                    emit(buf);

                    do_compile(ast->binary.right);

                    snprintf(buf, 256, "    jmp _while_%p\n", ast);
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
                case OP_RETURN: {
                    enum value v = do_compile(ast->unary.next);
                    if(v == VAL_PTR) emit("    mov rax, [rax]\n");
                    emit("    leave\n");
                    emit("    ret\n");
                    break;
                }
                case OP_IF: {
                    do_compile(ast->ternary.left);
                    emit("    test rax, rax\n");
                    char buf[256];
                    snprintf(buf, 256, "    jz _else_%p\n", ast);
                    emit(buf);
                    do_compile(ast->ternary.center);
                    snprintf(buf, 256, "    jmp _end_if_%p\n", ast);
                    emit(buf);
                    snprintf(buf, 256, "_else_%p:\n", ast);
                    emit(buf);
                    do_compile(ast->ternary.right);
                    snprintf(buf, 256, "_end_if_%p:\n", ast);
                    emit(buf);
                    break;
                }
                case OP_GOTO: {
                    char buf[256];
                    snprintf(buf, 256, "    jmp %s\n", ast->unary.next->ident.value);
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

                    emit("    test rax, rax\n");
                    snprintf(buf, 256, "    jz _end_for_%p\n", ast);
                    emit(buf);

                    do_compile(ast->binary.right);
                    do_compile(clause->ternary.right);

                    snprintf(buf, 256, "    jmp _for_%p\n", ast);
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
                    emit("    test rax, rax\n");
                    snprintf(buf, 256, "    jnz _dowhile_%p\n", ast);
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
                        char buf[256];
                        char* func_name = ast->ternary.left->binary.right->ident.value;
                        struct scoped_type* t = dict_get(
                            vars, func_name
                        );
                        t->replacement = strdup("fib");
                        t->replacement_type = VAL_PTR;
                        snprintf(buf, 256, "%s:\n",
                            func_name
                        );
                        emit(buf);
                        emit("push rbp\n");
                        emit("mov rbp, rsp\n");
                        struct ast* args = ast->ternary.center;
                        for(size_t i = 0; i < args->list.sz; i++) {
                            snprintf(buf, 256, 
                                "[rbp + %lu]",
                                i * 8 + 16
                            );
                            struct scoped_type* t = dict_get(
                                vars, 
                                args->list.list[i]->binary.right->ident.value
                            );
                            t->replacement = strdup(buf);
                            t->replacement_type = VAL_LITERAL;
                        }
                        for(size_t i = 0; i < body->list.sz; i++)
                            do_compile(body->list.list[i]);
                    }
                    break;
                }
                
                default:
                    break;
            }
    }
    return VAL_LITERAL;
}

void do_replace(char* data) {
    char* ptr = data;
    while((ptr = strstr(ptr, "_var_"))) {
        size_t idx = ptr - data;
        char* var = strdup(ptr);
        size_t len = strlen(var);
        for(size_t i = 0; i < len; i++) {
            if(var[i] == ' ' || var[i] == '\n') {
                var[i] = '\0';
                break;
            }
        }
        printf("Found var %s at index %ld\n", &var[5], idx);
        struct scoped_type* t = dict_get(vars, &var[5]);
        if(t && t->replacement) {
            size_t r_len = strlen(t->replacement);
            size_t var_len = strlen(var);
            size_t ptr_len = strlen(ptr);

            printf("r_len=%lu var_len=%lu ptr_len=%lu\n", r_len, var_len, ptr_len);
            printf("memmove %p %p %lu\n", ptr, &ptr[var_len], ptr_len - var_len);
            printf("memmove %p %p %lu\n", &ptr[r_len], ptr, ptr_len - r_len);
            printf("memcpy %p %p %lu\n", ptr, t->replacement, r_len);

            memmove(ptr, &ptr[var_len], ptr_len - var_len);
            memmove(&ptr[r_len], ptr, ptr_len - r_len);
            memcpy(ptr, t->replacement, r_len);
            ptr += r_len;
        } else ptr += strlen(var);
        free(var);
    }
}

void compile(struct ast* ast) {
    vars = get_idents(ast);

    size_t k_sz = vars->k->sz;
    size_t v_sz = vars->v->sz;
    size_t sz = (k_sz < v_sz) ? k_sz : v_sz;

    printf("{\n");
    for(size_t i = 0; i < sz; i++) {
        struct scoped_type* t = vars->v->data[i];
        printf("    %s: %s %p(%s)\n",
            (char*)vars->k->data[i], t->type, t->scope_top,
            op_strings[t->scope_top->unary.op]
        );
    }
    printf("}\n");
    do_compile(ast);
    fprintf(stderr, "%s", buffer);
}