#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "lexer.h"
#include "ast.h"

struct ast* expression();

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
    return false;
}

struct ast* factor() {
    if(accept(TOK_NUMBER)) {
        struct ast* ast = ast_new();
        ast->type = AST_NUMBER;
        ast->number.value = 0; // TODO: values
        return ast;
    }
    if(accept(TOK_IDENT)) {
        struct ast* ast = ast_new();
        ast->type = AST_IDENT;
        ast->ident.value = strdup("TODO: values");
        return ast;
    }
    if(accept(TOK_LPAREN)) {
        struct ast* ast = expression();
        expect(TOK_RPAREN);
        return ast;
    } else {
        error("Syntax error: invalid factor");
        lex_consume();
        return NULL;
    }
}

struct ast* term() {
    struct ast* left = factor();
    while(lex_peek() == TOK_TIMES || lex_peek() == TOK_SLASH) {
        enum token t = lex_consume();
        struct ast* new = ast_new();

        new->type = AST_BINARY;
        new->binary.tok = t;
        new->binary.left = left;
        new->binary.right = factor();

        left = new;
    }
    return left;
}

struct ast* expression() {
    struct ast* left = term();
    while(lex_peek() == TOK_PLUS || lex_peek() == TOK_MINUS) {
        enum token t = lex_consume();
        struct ast* new = ast_new();

        new->type = AST_BINARY;
        new->binary.tok = t;
        new->binary.left = left;
        new->binary.right = term();

        left = new;
    }
    return left;
}

struct ast* condition() {
    if (accept(TOK_ODD)) {
        return expression();
    }
    struct ast* left = expression();
    if (lex_peek() == TOK_EQ || lex_peek() == TOK_NEQ || lex_peek() == TOK_LESS || lex_peek() == TOK_LEQ || lex_peek() == TOK_GT || lex_peek() == TOK_GEQ) {
        enum token t = lex_consume();
        struct ast* new = ast_new();

        new->type = AST_BINARY;
        new->binary.tok = t;
        new->binary.left = left;
        new->binary.right = expression();
    } else {
        error("Syntax error: invalid operator in condition");
        lex_consume();
        return NULL;
    }
    return left;
}

struct ast* statement() {
    if(accept(TOK_IDENT)) {
        expect(TOK_BECOMES);
        struct ast* new = ast_new();

        new->type = AST_BINARY;
        new->binary.tok = TOK_BECOMES;

        new->binary.left = ast_new();
        new->binary.left->type = AST_IDENT;
        new->binary.left->ident.value = strdup("TODO: values");

        new->binary.right = expression();

        return new;
    } else if(accept(TOK_CALL)) {
        expect(TOK_IDENT);
        struct ast* new = ast_new();

        new->type = AST_BINARY;
        new->binary.tok = TOK_CALL;

        new->binary.left = ast_new();
        new->binary.left->type = AST_IDENT;
        new->binary.left->ident.value = strdup("TODO: values");

        return new;
    } else if(accept(TOK_BEGIN)) {
        struct ast* left = statement();
        while(accept(TOK_SEMICOLON)) {
            struct ast* new = ast_new();
            
            new->type = AST_BINARY;
            new->binary.tok = TOK_BEGIN;
            new->binary.left = left;
            new->binary.right = statement();
        }
        expect(TOK_END);
        return left;
    } else if(accept(TOK_IF)) {
        struct ast* new = ast_new();

        new->type = AST_BINARY;
        new->binary.tok = TOK_IF;

        new->binary.left = condition();
        expect(TOK_THEN);
        new->binary.right = statement();
        return new;
    } else if (accept(TOK_WHILE)) {
        struct ast* new = ast_new();

        new->type = AST_BINARY;
        new->binary.tok = TOK_WHILE;

        new->binary.left = condition();
        expect(TOK_DO);
        new->binary.right = statement();
        return new;
    } else {
        error("Syntax error: invalid statement");
        lex_consume();
        return NULL;
    }
}

struct ast* parse_const() {
    if(accept(TOK_CONST)) {
        struct ast* left = NULL;
        do {
            expect(TOK_IDENT);
            expect(TOK_EQ);
            expect(TOK_NUMBER);
            struct ast* new = ast_new();
            new->type = AST_BINARY;
            new->binary.tok = TOK_EQ;

            new->binary.left = ast_new();
            new->binary.left->type = AST_IDENT;
            new->binary.left->ident.value = strdup("TODO: values");

            new->binary.right = ast_new();
            new->binary.right->type = AST_NUMBER;
            new->binary.right->number.value = 0; // TODO: values

            struct ast* top = ast_new();
            top->type = AST_BINARY;
            top->binary.tok = TOK_CONST;

            top->binary.left = left;
            top->binary.right = new;

            left = top;
        } while(accept(TOK_COMMA));
        expect(TOK_SEMICOLON);
        return left;
    }

    return NULL;
}

struct ast* parse_var() {
    if (accept(TOK_VAR)) {
        struct ast* left = NULL;
        do {
            expect(TOK_IDENT);
            struct ast* new = ast_new();
            new->type = AST_BINARY;
            new->binary.tok = TOK_VAR;
            new->binary.left = left;

            new->binary.right = ast_new();
            new->binary.right->type = AST_IDENT;
            new->binary.right->ident.value = strdup("TODO: values");
            
            left = new;
        } while (accept(TOK_COMMA));
        expect(TOK_SEMICOLON);
        return left;
    }
    return NULL;
}

struct ast* block();

struct ast* parse_proc() {
    struct ast* left = NULL;
    while(accept(TOK_PROC)) {
        expect(TOK_IDENT);
        expect(TOK_SEMICOLON);
        struct ast* new = ast_new();
        new->type = AST_BINARY;
        new->binary.tok = TOK_PROC;
        new->binary.left = left;

        struct ast* right = ast_new();
        right->type = AST_BINARY;
        right->binary.tok = TOK_PROC;

        right->binary.left = ast_new();
        right->binary.left->type = AST_IDENT;
        right->binary.left->ident.value = strdup("TODO: values");

        right->binary.right = block();
        expect(TOK_SEMICOLON);

        new->binary.right = right;
        left = new;
    }
    return left;
}

struct ast* block() {
    struct ast* a = parse_const();
    a = parse_var();
    a = parse_proc();
    statement();
}

void program() {
    block();
    expect(TOK_PERIOD);
}