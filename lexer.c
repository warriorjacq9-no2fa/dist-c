#include "lexer.h"
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static size_t line, pos;
static char* data;
static size_t data_sz;
static size_t peek_pos;
static int num_val;
static char* ident_val;

enum token get_next_token(size_t* pos) {
top:
    if(*pos >= data_sz) return TOK_EOF;
    char c = data[(*pos)++];
    if(c == '\n') {
        goto top;
    }
    if(isspace(c)) goto top;
    if(c == '(') return TOK_LPAREN;
    if(c == ')') return TOK_RPAREN;
    if(c == '*') return TOK_TIMES;
    if(c == '/') return TOK_SLASH;
    if(c == '+') return TOK_PLUS;
    if(c == '-') return TOK_MINUS;
    if(c == ':' && data[*pos] == '=') {
        (*pos)++;
        return TOK_BECOMES;
    }
    if(c == '=') {
        if(data[*pos] == '=') {
            (*pos)++;
            return TOK_EQUAL;
        } else {
            return TOK_ASSGN;
        }
    }
    if(c == '#') return TOK_NEQ;
    if(c == '<') {
        if(data[*pos] == '=') {
            (*pos)++;
            return TOK_LEQ;
        } else return TOK_LESS;
    }
    if(c == '>') {
        if(data[*pos] == '=') {
            (*pos)++;
            return TOK_GEQ;
        } else return TOK_GT;
    }
    if(c == ';') return TOK_SEMICOLON;
    if(c == ',') return TOK_COMMA;
    if(c == '.') return TOK_PERIOD;
    (*pos)--;
    char tok[128];
    size_t i = *pos;
    char t = data[i];
    if(isdigit(t)) {
        while(1) {
            t = data[i];
            if(isdigit(t)) {
                tok[i - *pos] = t;
                i++;
            } else break;
        }
        tok[i - *pos] = '\0';
        *pos = i;
        num_val = atoi(tok);
        return TOK_NUMBER;
    }
    bool identifier = false;
    if(isalpha(t) || t == '_') {
        identifier = true;
        while(1) {
            t = data[i];
            if(isalpha(t) || isdigit(t) || t == '_') {
                tok[i - *pos] = t;
                i++;
            } else break;
        }
    }
    tok[i - *pos] = '\0';
    *pos = i;
    if(strcmp(tok, "begin") == 0) return TOK_BEGIN;
    if(strcmp(tok, "call") == 0) return TOK_CALL;
    if(strcmp(tok, "const") == 0) return TOK_CONST;
    if(strcmp(tok, "do") == 0) return TOK_DO;
    if(strcmp(tok, "end") == 0) return TOK_END;
    if(strcmp(tok, "if") == 0) return TOK_IF;
    if(strcmp(tok, "odd") == 0) return TOK_ODD;
    if(strcmp(tok, "procedure") == 0) return TOK_PROC;
    if(strcmp(tok, "then") == 0) return TOK_THEN;
    if(strcmp(tok, "var") == 0) return TOK_VAR;
    if(strcmp(tok, "while") == 0) return TOK_WHILE;

    if(identifier) {
        ident_val = strdup(tok);
        return TOK_IDENT;
    } else {
        char msg[256];
        snprintf(msg, 256, "Unkown token %s", tok);
        error(msg);
        return -1;
    }
}

int lex_get_number() {
    return num_val;
}

char* lex_get_ident() {
    return ident_val;
}

size_t get_line(size_t pos) {
    size_t lines = 0;
    for(size_t i = 0; i < pos; i++) {
        if(data[i] == '\n') lines++;
    }
    return lines + 1;
}

void error(const char* msg) {
    fprintf(stderr, "%s at line %lu\n", msg, peek_pos ? get_line(peek_pos) : get_line(pos));
    exit(1);
}

void lex_init(char* d) {
    data = d;
    data_sz = strlen(d);
    pos = 0;
    line = 1;
}

enum token lex_consume() {
    peek_pos = 0;
    enum token ret = get_next_token(&pos);
    return ret;
}

enum token lex_peek() {
    peek_pos = pos;
    enum token ret = get_next_token(&peek_pos);
    return ret;
}

const char* const token_strings[] = {
    "left parentheses",
    "right parentheses",
    "*",
    "/",
    "+",
    "-",
    ":=",
    "=",
    "==",
    "#",
    "<",
    "<=",
    ">",
    ">=",
    "semicolon",
    "comma",
    "period",
    "number",
    "begin",
    "call",
    "const",
    "do",
    "end",
    "if",
    "odd",
    "procedure",
    "then",
    "var",
    "while",
    "identifier",
    "end of file"
};