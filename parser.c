#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "lexer.h"
#include "ast.h"

struct ast* expr();

bool accept(enum token t) {
    if (lex_peek() == t) {
        lex_consume();
        return true;
    }
    return false;
}

bool expect(enum token t) {
    if (accept(t)) {
        return true;
    }
    char msg[64];
    enum token got = lex_peek();
    snprintf(msg, 64, "Unexpected %s (expected %s)", token_strings[got], token_strings[t]);
    error(msg);
}

struct ast* number() {
    expect(TOK_NUMBER);
    struct ast* ast = ast_new();
    ast->type = AST_NUMBER;
    ast->number.value = lex_get_number();
    return ast;
}

struct ast* ident() {
    expect(TOK_IDENT);
    struct ast* ast = ast_new();
    ast->type = AST_IDENT;
    ast->ident.value = lex_get_ident();
    return ast;
}

struct ast* type() {
    struct ast* ast = ast_new();
    ast->type = AST_BINARY;
    ast->binary.op = OP_TYPE;

    ast->binary.left = ident();

    int i = 0;
    while(accept(TOK_STAR)) i++;
    ast->binary.right = ast_new();
    ast->binary.right->type = AST_NUMBER;
    ast->binary.right->number.value = i;

    return ast;
}

struct ast* expr_0() {
    if(lex_peek() == TOK_IDENT) return ident();
    if(lex_peek() == TOK_NUMBER) return number();
    if(accept(TOK_LPAREN)) {
        struct ast* ast = expr();
        expect(TOK_RPAREN);
        return ast;
    }

    return NULL;
}

struct ast* expr_1() {
    struct ast* left = expr_0();
    if(left == NULL) return NULL;

    while(1) {
        switch(lex_peek()) {
            case TOK_INC: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_UNARY;
                new->unary.op = OP_POST_INC;
                new->unary.next = left;
                left = new;
                break;
            }
            case TOK_DEC: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_UNARY;
                new->unary.op = OP_POST_DEC;
                new->unary.next = left;
                left = new;
                break;
            }
            case TOK_LPAREN: {
                lex_consume();
                struct ast* arg = expr();
                if(arg) {
                    struct ast* arg_list = ast_new();
                    arg_list->type = AST_LIST;
                    arg_list->list.op = OP_CALL_ARGS;
                    
                    struct ast* args[256];
                    memset(args, 0, sizeof(args));
                    size_t i = 0;
                    args[i++] = arg;
                    while(accept(TOK_COMMA)) {
                        arg = expr();
                        if(arg == NULL) error("Expected expression");
                        args[i++] = arg;
                    }
                    expect(TOK_RPAREN);
                    arg_list->list.sz = i;
                    arg_list->list.list = malloc(i * sizeof(struct ast*));
                    memcpy(arg_list->list.list, args, i * sizeof(struct ast*));

                    struct ast* new = ast_new();
                    new->type = AST_BINARY;
                    new->binary.op = OP_CALL;
                    new->binary.left = left;
                    new->binary.right = arg_list;
                    left = new;
                } else {
                    struct ast* new = ast_new();
                    new->type = AST_UNARY;
                    new->unary.op = OP_CALL;
                    new->unary.next = left;
                    left = new;
                }
                break;
            }
            case TOK_LBRACKET: {
                lex_consume();
                struct ast* index = expr();
                if(index == NULL) error("Expected expression");
                expect(TOK_RBRACKET);

                struct ast* new = ast_new();
                new->type = AST_BINARY;
                new->binary.op = OP_ARRAY_DEREF;
                new->binary.left = left;
                new->binary.right = index;
                left = new;
                break;
            }
            default: return left;
        }
    }
}

void left_expand(struct ast** left, struct ast** bottom, struct ast* new) {
    if(*left) {
        switch((*bottom)->type) {
            case AST_UNARY:
                (*bottom)->unary.next = new;
                break;
            case AST_BINARY:
                (*bottom)->binary.right = new;
                break;
            default: break;
        }
    } else {
        *left = new;
    }
    *bottom = new;
}

struct ast* expr_2() {
    struct ast* left = NULL;
    struct ast* bottom = NULL;

    while(1) {
        switch(lex_peek()) {
            case TOK_INC: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_UNARY;
                new->unary.op = OP_PRE_INC;
                left_expand(&left, &bottom, new);
                break;
            }
            case TOK_DEC: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_UNARY;
                new->unary.op = OP_PRE_DEC;
                left_expand(&left, &bottom, new);
                break;
            }
            case TOK_PLUS: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_UNARY;
                new->unary.op = OP_UNARY_PLUS;
                left_expand(&left, &bottom, new);
                break;
            }
            case TOK_MINUS: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_UNARY;
                new->unary.op = OP_UNARY_MINUS;
                left_expand(&left, &bottom, new);
                break;
            }
            case TOK_BOOLINV: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_UNARY;
                new->unary.op = OP_BOOLINV;
                left_expand(&left, &bottom, new);
                break;
            }
            case TOK_LOGINV: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_UNARY;
                new->unary.op = OP_LOGINV;
                left_expand(&left, &bottom, new);
                break;
            }
            case TOK_STAR: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_UNARY;
                new->unary.op = OP_DEREF;
                left_expand(&left, &bottom, new);
                break;
            }
            case TOK_AMPERSAND: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_UNARY;
                new->unary.op = OP_REF;
                left_expand(&left, &bottom, new);
                break;
            }
            default: goto _next;
        }
    }
_next: {
        struct ast* right = expr_1();
        if(right == NULL) {
            ast_free(left);
            return NULL;
        }

        left_expand(&left, &bottom, right);
        return left;
    }
}

struct ast* expr_3() {
    struct ast* left = expr_2();
    if(left == NULL) return NULL;

    while(1) {
        switch(lex_peek()) {
            case TOK_STAR: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_BINARY;
                new->binary.op = OP_MUL;
                new->binary.left = left;
                struct ast* right = expr_2();
                if(right == NULL) error("Expected expression");
                new->binary.right = right;
                left = new;
                break;
            }
            case TOK_DIV: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_BINARY;
                new->binary.op = OP_DIV;
                new->binary.left = left;
                struct ast* right = expr_2();
                if(right == NULL) error("Expected expression");
                new->binary.right = right;
                left = new;
                break;
            }
            case TOK_MOD: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_BINARY;
                new->binary.op = OP_MOD;
                new->binary.left = left;
                struct ast* right = expr_2();
                if(right == NULL) error("Expected expression");
                new->binary.right = right;
                left = new;
                break;
            }
            default: return left;
        }
    }
}

struct ast* expr_4() {
    struct ast* left = expr_3();
    if(left == NULL) return NULL;

    while(1) {
        switch(lex_peek()) {
            case TOK_PLUS: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_BINARY;
                new->binary.op = OP_ADD;
                new->binary.left = left;
                struct ast* right = expr_3();
                if(right == NULL) error("Expected expression");
                new->binary.right = right;
                left = new;
                break;
            }
            case TOK_MINUS: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_BINARY;
                new->binary.op = OP_SUB;
                new->binary.left = left;
                struct ast* right = expr_3();
                if(right == NULL) error("Expected expression");
                new->binary.right = right;
                left = new;
                break;
            }
            default: return left;
        }
    }
}

struct ast* expr_5() {
    struct ast* left = expr_4();
    if(left == NULL) return NULL;

    while(1) {
        switch(lex_peek()) {
            case TOK_LSHIFT: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_BINARY;
                new->binary.op = OP_LSHIFT;
                new->binary.left = left;
                struct ast* right = expr_4();
                if(right == NULL) error("Expected expression");
                new->binary.right = right;
                left = new;
                break;
            }
            case TOK_RSHIFT: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_BINARY;
                new->binary.op = OP_RSHIFT;
                new->binary.left = left;
                struct ast* right = expr_4();
                if(right == NULL) error("Expected expression");
                new->binary.right = right;
                left = new;
                break;
            }
            default: return left;
        }
    }
}

struct ast* expr_6() {
    struct ast* left = expr_5();
    if(left == NULL) return NULL;

    while(1) {
        switch(lex_peek()) {
            case TOK_LESS: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_BINARY;
                new->binary.op = OP_LESS;
                new->binary.left = left;
                struct ast* right = expr_5();
                if(right == NULL) error("Expected expression");
                new->binary.right = right;
                left = new;
                break;
            }
            case TOK_LEQ: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_BINARY;
                new->binary.op = OP_LEQ;
                new->binary.left = left;
                struct ast* right = expr_5();
                if(right == NULL) error("Expected expression");
                new->binary.right = right;
                left = new;
                break;
            }
            case TOK_GREATER: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_BINARY;
                new->binary.op = OP_GREATER;
                new->binary.left = left;
                struct ast* right = expr_5();
                if(right == NULL) error("Expected expression");
                new->binary.right = right;
                left = new;
                break;
            }
            case TOK_GEQ: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_BINARY;
                new->binary.op = OP_GEQ;
                new->binary.left = left;
                struct ast* right = expr_5();
                if(right == NULL) error("Expected expression");
                new->binary.right = right;
                left = new;
                break;
            }
            default: return left;
        }
    }
}

struct ast* expr_7() {
    struct ast* left = expr_6();
    if(left == NULL) return NULL;

    while(1) {
        switch(lex_peek()) {
            case TOK_EQUALTO: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_BINARY;
                new->binary.op = OP_EQUALTO;
                new->binary.left = left;
                struct ast* right = expr_6();
                if(right == NULL) error("Expected expression");
                new->binary.right = right;
                left = new;
                break;
            }
            case TOK_NEQ: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_BINARY;
                new->binary.op = OP_NEQ;
                new->binary.left = left;
                struct ast* right = expr_6();
                if(right == NULL) error("Expected expression");
                new->binary.right = right;
                left = new;
                break;
            }
            default: return left;
        }
    }
}

struct ast* expr_8() {
    struct ast* left = expr_7();
    if(left == NULL) return NULL;

    while(1) {
        switch(lex_peek()) {
            case TOK_AMPERSAND: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_BINARY;
                new->binary.op = OP_AND;
                new->binary.left = left;
                struct ast* right = expr_7();
                if(right == NULL) error("Expected expression");
                new->binary.right = right;
                left = new;
                break;
            }
            default: return left;
        }
    }
}

struct ast* expr_9() {
    struct ast* left = expr_8();
    if(left == NULL) return NULL;

    while(1) {
        switch(lex_peek()) {
            case TOK_XOR: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_BINARY;
                new->binary.op = OP_XOR;
                new->binary.left = left;
                struct ast* right = expr_8();
                if(right == NULL) error("Expected expression");
                new->binary.right = right;
                left = new;
                break;
            }
            default: return left;
        }
    }
}

struct ast* expr_10() {
    struct ast* left = expr_9();
    if(left == NULL) return NULL;

    while(1) {
        switch(lex_peek()) {
            case TOK_OR: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_BINARY;
                new->binary.op = OP_OR;
                new->binary.left = left;
                struct ast* right = expr_9();
                if(right == NULL) error("Expected expression");
                new->binary.right = right;
                left = new;
                break;
            }
            default: return left;
        }
    }
}

struct ast* expr_11() {
    struct ast* left = expr_10();
    if(left == NULL) return NULL;

    while(1) {
        switch(lex_peek()) {
            case TOK_BOOLAND: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_BINARY;
                new->binary.op = OP_BOOLAND;
                new->binary.left = left;
                struct ast* right = expr_10();
                if(right == NULL) error("Expected expression");
                new->binary.right = right;
                left = new;
                break;
            }
            default: return left;
        }
    }
}

struct ast* expr_12() {
    struct ast* left = expr_11();
    if(left == NULL) return NULL;

    while(1) {
        switch(lex_peek()) {
            case TOK_BOOLOR: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_BINARY;
                new->binary.op = OP_BOOLOR;
                new->binary.left = left;
                struct ast* right = expr_11();
                if(right == NULL) error("Expected expression");
                new->binary.right = right;
                left = new;
                break;
            }
            default: return left;
        }
    }
}

struct ast* expr_13() {
    struct ast* left = expr_12();
    if(left == NULL) return NULL;

    if(lex_peek() == TOK_TERNARYIF) {
        lex_consume();
        struct ast* a_if = expr_12();
        if(a_if == NULL) error("Expected expression");
        expect(TOK_COLON);
        struct ast* a_else = expr_13();
        if(a_else == NULL) error("Expected expression");

        struct ast* res = ast_new();
        res->type = AST_BINARY;
        res->binary.op = OP_TERNARY_RES;
        res->binary.left = a_if;
        res->binary.right = a_else;

        struct ast* new = ast_new();
        new->type = AST_BINARY;
        new->binary.op = OP_TERNARY;
        new->binary.left = left;
        new->binary.right = res;

        left = new;
    }
    return left;
}
struct ast* expr_14() {
    struct ast* left = expr_13();
    if(left == NULL) return NULL;

    switch(lex_peek()) {
        case TOK_EQUALS: {
            lex_consume();
            struct ast* new = ast_new();
            new->type = AST_BINARY;
            new->binary.op = OP_ASSIGN;
            new->binary.left = left;
            struct ast* right = expr_14();
            if(right == NULL) error("Expected expression");
            new->binary.right = right;
            left = new;
            break;
        }
        case TOK_EQPLUS: {
            lex_consume();
            struct ast* new = ast_new();
            new->type = AST_BINARY;
            new->binary.op = OP_ASSIGN;
            new->binary.left = left;
        
            struct ast* val = expr_14();
            if(val == NULL) error("Expected expression");
            struct ast* right = ast_new();
            right->type = AST_BINARY;
            right->binary.op = OP_ADD;
            right->binary.left = left;
            right->binary.right = val;

            new->binary.right = right;
            left = new;
            break;
        }
        case TOK_EQMINUS: {
            lex_consume();
            struct ast* new = ast_new();
            new->type = AST_BINARY;
            new->binary.op = OP_ASSIGN;
            new->binary.left = left;
        
            struct ast* val = expr_14();
            if(val == NULL) error("Expected expression");
            struct ast* right = ast_new();
            right->type = AST_BINARY;
            right->binary.op = OP_SUB;
            right->binary.left = left;
            right->binary.right = val;

            new->binary.right = right;
            left = new;
            break;
        }
        case TOK_EQMUL: {
            lex_consume();
            struct ast* new = ast_new();
            new->type = AST_BINARY;
            new->binary.op = OP_ASSIGN;
            new->binary.left = left;
        
            struct ast* val = expr_14();
            if(val == NULL) error("Expected expression");
            struct ast* right = ast_new();
            right->type = AST_BINARY;
            right->binary.op = OP_MUL;
            right->binary.left = left;
            right->binary.right = val;

            new->binary.right = right;
            left = new;
            break;
        }
        case TOK_EQDIV: {
            lex_consume();
            struct ast* new = ast_new();
            new->type = AST_BINARY;
            new->binary.op = OP_ASSIGN;
            new->binary.left = left;
        
            struct ast* val = expr_14();
            if(val == NULL) error("Expected expression");
            struct ast* right = ast_new();
            right->type = AST_BINARY;
            right->binary.op = OP_DIV;
            right->binary.left = left;
            right->binary.right = val;

            new->binary.right = right;
            left = new;
            break;
        }
        case TOK_EQMOD: {
            lex_consume();
            struct ast* new = ast_new();
            new->type = AST_BINARY;
            new->binary.op = OP_ASSIGN;
            new->binary.left = left;
        
            struct ast* val = expr_14();
            if(val == NULL) error("Expected expression");
            struct ast* right = ast_new();
            right->type = AST_BINARY;
            right->binary.op = OP_MOD;
            right->binary.left = left;
            right->binary.right = val;

            new->binary.right = right;
            left = new;
            break;
        }
        case TOK_EQLSHIFT: {
            lex_consume();
            struct ast* new = ast_new();
            new->type = AST_BINARY;
            new->binary.op = OP_ASSIGN;
            new->binary.left = left;
        
            struct ast* val = expr_14();
            if(val == NULL) error("Expected expression");
            struct ast* right = ast_new();
            right->type = AST_BINARY;
            right->binary.op = OP_LSHIFT;
            right->binary.left = left;
            right->binary.right = val;

            new->binary.right = right;
            left = new;
            break;
        }
        case TOK_EQRSHIFT: {
            lex_consume();
            struct ast* new = ast_new();
            new->type = AST_BINARY;
            new->binary.op = OP_ASSIGN;
            new->binary.left = left;
        
            struct ast* val = expr_14();
            if(val == NULL) error("Expected expression");
            struct ast* right = ast_new();
            right->type = AST_BINARY;
            right->binary.op = OP_RSHIFT;
            right->binary.left = left;
            right->binary.right = val;

            new->binary.right = right;
            left = new;
            break;
        }
        case TOK_EQAND: {
            lex_consume();
            struct ast* new = ast_new();
            new->type = AST_BINARY;
            new->binary.op = OP_ASSIGN;
            new->binary.left = left;
        
            struct ast* val = expr_14();
            if(val == NULL) error("Expected expression");
            struct ast* right = ast_new();
            right->type = AST_BINARY;
            right->binary.op = OP_AND;
            right->binary.left = left;
            right->binary.right = val;

            new->binary.right = right;
            left = new;
            break;
        }
        case TOK_EQXOR: {
            lex_consume();
            struct ast* new = ast_new();
            new->type = AST_BINARY;
            new->binary.op = OP_ASSIGN;
            new->binary.left = left;
        
            struct ast* val = expr_14();
            if(val == NULL) error("Expected expression");
            struct ast* right = ast_new();
            right->type = AST_BINARY;
            right->binary.op = OP_XOR;
            right->binary.left = left;
            right->binary.right = val;

            new->binary.right = right;
            left = new;
            break;
        }
        case TOK_EQOR: {
            lex_consume();
            struct ast* new = ast_new();
            new->type = AST_BINARY;
            new->binary.op = OP_ASSIGN;
            new->binary.left = left;
        
            struct ast* val = expr_14();
            if(val == NULL) error("Expected expression");
            struct ast* right = ast_new();
            right->type = AST_BINARY;
            right->binary.op = OP_OR;
            right->binary.left = left;
            right->binary.right = val;

            new->binary.right = right;
            left = new;
            break;
        }
        default: break;
    }
    return left;
}

struct ast* expr_15() {
    struct ast* left = expr_14();
    if(left == NULL) return NULL;

    while(1) {
        switch(lex_peek()) {
            case TOK_COMMA: {
                lex_consume();
                struct ast* new = ast_new();
                new->type = AST_BINARY;
                new->binary.op = OP_COMMA;
                new->binary.left = left;
                struct ast* right = expr_11();
                if(right == NULL) error("Expected expression");
                new->binary.right = right;
                left = new;
                break;
            }
            default: return left;
        }
    }
}

struct ast* expr() {
    return expr_15();
}