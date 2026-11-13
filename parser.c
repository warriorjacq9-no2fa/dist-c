#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "lexer.h"
#include "ast.h"

struct ast* expr();
struct ast* stmt();

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
    if(accept(TOK_NUMBER)) {
        struct ast* ast = ast_new();
        ast->type = AST_NUMBER;
        ast->number.value = lex_get_number();
        return ast;
    }
    return NULL;
}

struct ast* ident() {
    if(accept(TOK_IDENT)) {
        struct ast* ast = ast_new();
        ast->type = AST_IDENT;
        ast->ident.value = lex_get_ident();
        return ast;
    }
    return NULL;
}

struct ast* type() {
    struct ast* id = ident();
    if(id == NULL) return NULL;
    struct ast* ast = ast_new();
    ast->type = AST_UNARY;
    ast->unary.op = OP_TYPE;

    char data[256];
    strcpy(data, id->ident.value);
    ast_free(id);

    int i = strlen(data);
    while(accept(TOK_STAR)) {
        data[i] = '*';
        i++;
    }
    ast->unary.next = ast_new();
    ast->unary.next->type = AST_IDENT;
    ast->unary.next->ident.value = strdup(data);

    return ast;
}

struct ast* expr_0() {
    struct ast* ast;
    if((ast = ident())) return ast;
    if((ast = number())) return ast;
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

struct ast* expr_2() {
    struct ast* left = NULL;

    switch(lex_peek()) {
        case TOK_INC: {
            lex_consume();
            struct ast* new = ast_new();
            new->type = AST_UNARY;
            new->unary.op = OP_PRE_INC;
            new->unary.next = expr_2();
            left = new;
            break;
        }
        case TOK_DEC: {
            lex_consume();
            struct ast* new = ast_new();
            new->type = AST_UNARY;
            new->unary.op = OP_PRE_DEC;
            new->unary.next = expr_2();
            left = new;
            break;
        }
        case TOK_PLUS: {
            lex_consume();
            struct ast* new = ast_new();
            new->type = AST_UNARY;
            new->unary.op = OP_UNARY_PLUS;
            new->unary.next = expr_2();
            left = new;
            break;
        }
        case TOK_MINUS: {
            lex_consume();
            struct ast* new = ast_new();
            new->type = AST_UNARY;
            new->unary.op = OP_UNARY_MINUS;
            new->unary.next = expr_2();
            left = new;
            break;
        }
        case TOK_BOOLINV: {
            lex_consume();
            struct ast* new = ast_new();
            new->type = AST_UNARY;
            new->unary.op = OP_BOOLINV;
            new->unary.next = expr_2();
            left = new;
            break;
        }
        case TOK_LOGINV: {
            lex_consume();
            struct ast* new = ast_new();
            new->type = AST_UNARY;
            new->unary.op = OP_LOGINV;
            new->unary.next = expr_2();
            left = new;
            break;
        }
        case TOK_STAR: {
            lex_consume();
            struct ast* new = ast_new();
            new->type = AST_UNARY;
            new->unary.op = OP_DEREF;
            new->unary.next = expr_2();
            left = new;
            break;
        }
        case TOK_AMPERSAND: {
            lex_consume();
            struct ast* new = ast_new();
            new->type = AST_UNARY;
            new->unary.op = OP_REF;
            new->unary.next = expr_2();
            left = new;
            break;
        }
        default: return expr_1();
    }
    
    return left;
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

        struct ast* new = ast_new();
        new->type = AST_TERNARY;
        new->ternary.op = OP_TERNARY;
        new->ternary.left = left;
        new->ternary.center = a_if;
        new->ternary.right = a_else;

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
                struct ast* right = expr_14();
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

struct ast* parse_while() {
    if(accept(TOK_WHILE)) {
        expect(TOK_LPAREN);
        struct ast* cond = expr();
        expect(TOK_RPAREN);
        struct ast* top = ast_new();
        top->type = AST_BINARY;
        top->binary.op = OP_WHILE;
        top->binary.left = cond;
        top->binary.right = stmt();
        return top;
    }
    return NULL;
}

struct ast* parse_default() {
    if(accept(TOK_DEFAULT)) {
        expect(TOK_COLON);
        struct ast* new = ast_new();
        new->type = AST_UNARY;
        new->unary.op = OP_DEFAULT;
        return new;
    }
    return NULL;
}

struct ast* parse_case() {
    if(accept(TOK_CASE)) {
        struct ast* e = expr();
        if(e == NULL) error("Expected expression");
        expect(TOK_COLON);

        struct ast* new = ast_new();
        new->type = AST_UNARY;
        new->unary.op = OP_CASE;
        new->unary.next = e;

        return new;
    }
    return NULL;
}

struct ast* parse_switch() {
    if(accept(TOK_SWITCH)) {
        expect(TOK_LPAREN);
        struct ast* e = expr();
        if(e == NULL) error("Expected expression");
        expect(TOK_RPAREN);
        expect(TOK_LBRACE);

        struct ast* body = ast_new();
        body->type = AST_LIST;
        body->list.op = OP_SWITCH_BODY;
        struct ast* labels[1024];
        size_t sz = 0;
        while(1) {
            labels[sz] = parse_case();
            if(labels[sz] == NULL) labels[sz] = parse_default();
            if(labels[sz] == NULL) labels[sz] = stmt();
            if(labels[sz] == NULL) break;
            else sz++;
        }

        expect(TOK_RBRACE);

        body->list.list = malloc(sz * sizeof(struct ast*));
        memcpy(body->list.list, labels, sz * sizeof(struct ast*));
        body->list.sz = sz;

        struct ast* top = ast_new();
        top->type = AST_BINARY;
        top->binary.op = OP_SWITCH;
        top->binary.left = e;
        top->binary.right = body;

        return top;
    }
    return NULL;
}

struct ast* parse_return() {
    if(accept(TOK_RETURN)) {
        struct ast* new = ast_new();
        new->type = AST_UNARY;
        new->unary.op = OP_RETURN;
        new->unary.next = expr();
        expect(TOK_SEMICOLON);
        return new;
    }
    return NULL;
}

struct ast* parse_if() {
    if(accept(TOK_IF)) {
        expect(TOK_LPAREN);
        struct ast* e = expr();
        if(e == NULL) error("Expected expression");
        expect(TOK_RPAREN);

        struct ast* body = stmt();
        struct ast* else_body = NULL;

        if(accept(TOK_ELSE)) else_body = stmt();

        struct ast* top = ast_new();
        top->type = AST_TERNARY;
        top->ternary.op = OP_IF;
        top->ternary.left = e;
        top->ternary.center = body;
        top->ternary.right = else_body;

        return top;
    }
    return NULL;
}

struct ast* parse_goto() {
    if(accept(TOK_GOTO)) {
        struct ast* i = ident();
        expect(TOK_SEMICOLON);
        struct ast* new = ast_new();
        new->type = AST_UNARY;
        new->unary.op = OP_GOTO;
        new->unary.next = i;
        return new;
    }
    return NULL;
}

struct ast* var_decl() {
    size_t pos = lex_get_pos();
    struct ast* t = type();
    if(t == NULL) return NULL;
    struct ast* id = ident();
    if(id == NULL) {
        lex_set_pos(pos);
        ast_free(t);
        return NULL;
    }
    struct ast* arr = NULL;
    struct ast* val = NULL;
    if(accept(TOK_LBRACKET)) {
        arr = expr();
        expect(TOK_RBRACKET);
    }
    if(accept(TOK_EQUALS)) {
        val = expr();
    }

    struct ast* proto = ast_new();
    proto->type = AST_BINARY;
    proto->binary.op = OP_VAR_PROTO;
    proto->binary.left = t;
    proto->binary.right = id;

    if(arr || val) {
        struct ast* body = ast_new();
        body->type = AST_BINARY;
        body->binary.op = OP_VAR_BODY;
        body->binary.left = arr;
        body->binary.right = val;
        
        struct ast* new = ast_new();
        new->type = AST_BINARY;
        new->binary.op = OP_VAR;
        new->binary.left = proto;
        new->binary.right = body;
        return new;
    }
    return proto;
}

struct ast* parse_for() {
    if(accept(TOK_FOR)) {
        expect(TOK_LPAREN);
        struct ast* a1 = var_decl();
        if(a1 == NULL) a1 = expr();
        expect(TOK_SEMICOLON);
        struct ast* a2 = expr();
        expect(TOK_SEMICOLON);
        struct ast* a3 = expr();
        expect(TOK_RPAREN);
        struct ast* s = stmt();

        struct ast* clause = ast_new();
        clause->type = AST_TERNARY;
        clause->ternary.op = OP_FOR_CLAUSE;
        clause->ternary.left = a1;
        clause->ternary.center = a2;
        clause->ternary.right = a3;

        struct ast* top = ast_new();
        top->type = AST_BINARY;
        top->binary.op = OP_FOR;
        top->binary.left = clause;
        top->binary.right = s;

        return top;
    }
    return NULL;
}

struct ast* do_while() {
    if(accept(TOK_DO)) {
        struct ast* s = stmt();
        expect(TOK_WHILE);
        expect(TOK_LPAREN);
        struct ast* e = expr();
        if(e == NULL) error("Expected expression");
        expect(TOK_RPAREN);
        expect(TOK_SEMICOLON);

        struct ast* new = ast_new();
        new->type = AST_BINARY;
        new->binary.op = OP_DO_WHILE;
        new->binary.left = s;
        new->binary.right = e;

        return new;
    }
    return NULL;
}

struct ast* parse_break() {
    if(accept(TOK_BREAK)) {
        expect(TOK_SEMICOLON);
        struct ast* new = ast_new();
        new->type = AST_UNARY;
        new->unary.op = OP_BREAK;
        return new;
    }
    return NULL;
}

struct ast* stmt() {
    struct ast* a;
    if((a = var_decl())) {
        expect(TOK_SEMICOLON);
        return a;
    }
    if((a = expr())) {
        expect(TOK_SEMICOLON);
        return a;
    }
    if((a = parse_break())) return a;
    if((a = do_while())) return a;
    if((a = parse_for())) return a;
    if((a = parse_goto())) return a;
    if((a = parse_if())) return a;
    if((a = parse_return())) return a;
    if((a = parse_switch())) return a;
    if((a = parse_while())) return a;
    if(accept(TOK_LBRACE)) {
        struct ast* body = ast_new();
        body->type = AST_LIST;
        body->list.op = OP_STMT_BODY;
        struct ast* stmts[1024];
        size_t sz = 0;
        while((stmts[sz] = stmt())) sz++;
        body->list.list = malloc(sz * sizeof(struct ast*));
        memcpy(body->list.list, stmts, sz * sizeof(struct ast*));
        body->list.sz = sz;
        expect(TOK_RBRACE);
        return body;
    }
    return NULL;
}

struct ast* arg() {
    struct ast* t = type();
    if(t == NULL) return NULL;
    struct ast* id = ident();
    if(id == NULL) error("Expected identifier");
    
    struct ast* new = ast_new();
    new->type = AST_BINARY;
    new->binary.op = OP_ARG;
    new->binary.left = t;
    new->binary.right = id;

    return new;
}

struct ast* arg_list() {
    struct ast* list = ast_new();
    list->type = AST_LIST;
    list->list.op = OP_ARG_LIST;
    struct ast* stmts[1024];
    size_t sz = 0;
    while(1) {
        stmts[sz] = arg();
        if(stmts[sz] == NULL) break;
        sz++;
        if(!accept(TOK_COMMA)) break;
    }
    list->list.list = malloc(sz * sizeof(struct ast*));
    memcpy(list->list.list, stmts, sz * sizeof(struct ast*));
    list->list.sz = sz;
    return list;
}

struct ast* tmp();

struct ast* decl() {
    struct ast* t = type();
    if(t == NULL) return NULL;
    struct ast* id = ident();
    if(id == NULL) error("Expected identifier");
    if(accept(TOK_LPAREN)) { // function
        struct ast* args = arg_list();
        expect(TOK_RPAREN);
        struct ast* new = ast_new();
        new->type = AST_TERNARY;
        new->ternary.op = OP_FUNC_PROTO;
        new->ternary.left = t;
        new->ternary.center = id;
        new->ternary.right = args;

        if(accept(TOK_LBRACE)) {
            struct ast* top = ast_new();
            top->type = AST_BINARY;
            top->binary.op = OP_FUNC;
            top->binary.left = new;

            struct ast* body = ast_new();
            body->type = AST_LIST;
            body->list.op = OP_FUNC_BODY;
            struct ast* stmts[1024];
            size_t sz = 0;
            while((stmts[sz] = stmt())) sz++;
            body->list.list = malloc(sz * sizeof(struct ast*));
            memcpy(body->list.list, stmts, sz * sizeof(struct ast*));
            body->list.sz = sz;
            expect(TOK_RBRACE);
            top->binary.right = body;
            return top;
        }
        expect(TOK_SEMICOLON);

        return new;
    } else { // not a function
        struct ast* arr = NULL;
        struct ast* val = NULL;
        if(accept(TOK_LBRACKET)) {
            arr = expr();
            expect(TOK_RBRACKET);
        }
        if(accept(TOK_EQUALS)) {
            val = expr();
        }

        struct ast* proto = ast_new();
        proto->type = AST_BINARY;
        proto->binary.op = OP_VAR_PROTO;
        proto->binary.left = t;
        proto->binary.right = id;

        if(arr || val) {
            struct ast* body = ast_new();
            body->type = AST_BINARY;
            body->binary.op = OP_VAR_BODY;
            body->binary.left = arr;
            body->binary.right = val;
            
            struct ast* new = ast_new();
            new->type = AST_BINARY;
            new->binary.op = OP_VAR;
            new->binary.left = proto;
            new->binary.right = body;
            return new;
        }
        return proto;
    }
    return NULL;
}

struct ast* program() {
    struct ast* body = ast_new();
    body->type = AST_LIST;
    body->list.op = OP_PROGRAM;
    struct ast* decls[1024];
    size_t sz = 0;
    while((decls[sz] = decl())) sz++;
    body->list.list = malloc(sz * sizeof(struct ast*));
    memcpy(body->list.list, decls, sz * sizeof(struct ast*));
    body->list.sz = sz;

    return body;
}