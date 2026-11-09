#include <stdio.h>
#include <stdbool.h>
#include "lexer.h"

void expression();

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

void factor() {
    if (accept(TOK_IDENT) || accept(TOK_NUMBER)) {
        return;
    }
    if (accept(TOK_LPAREN)) {
        expression();
        expect(TOK_RPAREN);
    } else {
        error("Syntax error: invalid factor");
        lex_consume();
    }
}

void term() {
    factor();
    while (lex_peek() == TOK_TIMES || lex_peek() == TOK_SLASH) {
        lex_consume();
        factor();
    }
}

void expression() {
    if (lex_peek() == TOK_PLUS || lex_peek() == TOK_MINUS) {
        lex_consume();
    }
    term();
    while (lex_peek() == TOK_PLUS || lex_peek() == TOK_MINUS) {
        lex_consume();
        term();
    }
}

void condition() {
    if (accept(TOK_ODD)) {
        expression();
        return;
    }
    expression();
    if (lex_peek() == TOK_EQ || lex_peek() == TOK_NEQ || lex_peek() == TOK_LESS || lex_peek() == TOK_LEQ || lex_peek() == TOK_GT || lex_peek() == TOK_GEQ) {
        lex_consume();
        expression();
    } else {
        error("Syntax error: invalid operator in condition");
        lex_consume();
    }
}

void statement() {
    if (accept(TOK_IDENT)) {
        expect(TOK_BECOMES);
        expression();
    } else if (accept(TOK_CALL)) {
        expect(TOK_IDENT);
    } else if (accept(TOK_BEGIN)) {
        do {
            statement();
        } while (accept(TOK_SEMICOLON));
        expect(TOK_END);
    } else if (accept(TOK_IF)) {
        condition();
        expect(TOK_THEN);
        statement();
    } else if (accept(TOK_WHILE)) {
        condition();
        expect(TOK_DO);
        statement();
    } else {
        error("Syntax error: invalid statement");
        lex_consume();
    }
}

void block() {
    if (accept(TOK_CONST)) {
        do {
            expect(TOK_IDENT);
            expect(TOK_EQ);
            expect(TOK_NUMBER);
        } while (accept(TOK_COMMA));
        expect(TOK_SEMICOLON);
    }
    if (accept(TOK_VAR)) {
        do {
            expect(TOK_IDENT);
        } while (accept(TOK_COMMA));
        expect(TOK_SEMICOLON);
    }
    while (accept(TOK_PROC)) {
        expect(TOK_IDENT);
        expect(TOK_SEMICOLON);
        block();
        expect(TOK_SEMICOLON);
    }
    statement();
}

void program() {
    block();
    expect(TOK_PERIOD);
}