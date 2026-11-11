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
    if(c == '[') return TOK_LBRACKET;
    if(c == ']') return TOK_RBRACKET;
    if(c == '{') return TOK_LBRACE;
    if(c == '}') return TOK_RBRACE;
    if(c == '~') return TOK_LOGINV;
    if(c == ',') return TOK_COMMA;
    if(c == ';') return TOK_SEMICOLON;
    if(c == ':') return TOK_COLON;
    if(c == '?') return TOK_TERNARYIF;
    if(c == '!') {
        if(*pos < data_sz && data[*pos] == '=') {
            (*pos)++;
            return TOK_NEQ;
        }
        return TOK_BOOLINV;
    }
    if(c == '+') {
        if(*pos < data_sz) {
            if(data[*pos] == '=') {
                (*pos)++;
                return TOK_EQPLUS;
            }
            if(data[*pos] == '+') {
                (*pos)++;
                return TOK_INC;
            }
        }
        return TOK_PLUS;
    }
    if(c == '-') {
        if(*pos < data_sz) {
            if(data[*pos] == '=') {
                (*pos)++;
                return TOK_EQMINUS;
            }
            if(data[*pos] == '-') {
                (*pos)++;
                return TOK_DEC;
            }
        }
        return TOK_MINUS;
    }
    if(c == '*') {
        if(*pos < data_sz && data[*pos] == '=') {
            (*pos)++;
            return TOK_EQMUL;
        }
        return TOK_STAR;
    }
    if(c == '/') {
        if(*pos < data_sz && data[*pos] == '=') {
            (*pos)++;
            return TOK_EQDIV;
        }
        return TOK_DIV;
    }
    if(c == '%') {
        if(*pos < data_sz && data[*pos] == '=') {
            (*pos)++;
            return TOK_EQMOD;
        }
        return TOK_MOD;
    }
    if(c == '&') {
        if(*pos < data_sz) {
            if(data[*pos] == '&') {
                (*pos)++;
                return TOK_BOOLAND;
            }
            if(data[*pos] == '=') {
                (*pos)++;
                return TOK_EQAND;
            }
        }
        return TOK_AMPERSAND;
    }
    if(c == '^') {
        if(*pos < data_sz && data[*pos] == '=') {
            (*pos)++;
            return TOK_EQXOR;
        }
        return TOK_XOR;
    }
    if(c == '|') {
        if(*pos < data_sz) {
            if(data[*pos] == '|') {
                (*pos)++;
                return TOK_BOOLOR;
            }
            if(data[*pos] == '=') {
                (*pos)++;
                return TOK_EQOR;
            }
        }
        return TOK_OR;
    }
    if(c == '<') {
        if(*pos < data_sz) {
            if(data[*pos] == '<') {
                (*pos)++;
                if(*pos < data_sz && data[*pos] == '=') {
                    (*pos)++;
                    return TOK_EQLSHIFT;
                }
                return TOK_LSHIFT;
            }
            if(data[*pos] == '=') {
                (*pos)++;
                return TOK_LEQ;
            }
        }
        return TOK_LESS;
    }
    if(c == '>') {
        if(*pos < data_sz) {
            if(data[*pos] == '>') {
                (*pos)++;
                if(*pos < data_sz && data[*pos] == '=') {
                    (*pos)++;
                    return TOK_EQRSHIFT;
                }
                return TOK_RSHIFT;
            }
            if(data[*pos] == '=') {
                (*pos)++;
                return TOK_GEQ;
            }
        }
        return TOK_GREATER;
    }
    if(c == '=') {
        if(*pos < data_sz && data[*pos] == '=') {
            (*pos)++;
            return TOK_EQUALTO;
        }
        return TOK_EQUALS;
    }
    if(isdigit(c)) {
        char tok[128];
        size_t i = 0;
        tok[i++] = c;
        while(*pos < data_sz && isdigit(data[*pos])) {
            tok[i++] = data[(*pos)++];
        }
        tok[i] = '\0';
        num_val = atoi(tok);
        return TOK_NUMBER;
    }
    if(isalpha(c) || c == '_') {
        char tok[128];
        size_t i = 0;
        tok[i++] = c;
        while(*pos < data_sz &&
            (isalpha(data[*pos]) || isdigit(data[*pos]) || data[*pos] == '_')
        ) {
            tok[i++] = data[(*pos)++];
        }
        tok[i] = '\0';
        
        if(strcmp(tok, "break") == 0) return TOK_BREAK;
        if(strcmp(tok, "case") == 0) return TOK_CASE;
        if(strcmp(tok, "default") == 0) return TOK_DEFAULT;
        if(strcmp(tok, "do") == 0) return TOK_DO;
        if(strcmp(tok, "else") == 0) return TOK_ELSE;
        if(strcmp(tok, "for") == 0) return TOK_FOR;
        if(strcmp(tok, "goto") == 0) return TOK_GOTO;
        if(strcmp(tok, "if") == 0) return TOK_IF;
        if(strcmp(tok, "return") == 0) return TOK_RETURN;
        if(strcmp(tok, "switch") == 0) return TOK_SWITCH;
        if(strcmp(tok, "while") == 0) return TOK_WHILE;

        ident_val = strdup(tok);
        return TOK_IDENT;
    }

    error("Unkown token");
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

[[noreturn]] void error(const char* msg) {
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
    "left bracket",
    "right bracket",
    "left brace",
    "right brace",
    "logical invert",
    "comma",
    "semicolon",
    "colon",
    "question mark",
    "boolean invert",
    "not equal",
    "plus",
    "plus-equals",
    "increment",
    "minus",
    "minus-equals",
    "decrement",
    "star",
    "times-equals",
    "divide",
    "divide-equals",
    "modulus",
    "modulus-equals",
    "ampersand",
    "boolean and",
    "and-equals",
    "xor",
    "xor-equals",
    "or",
    "boolean or",
    "or-equals",
    "less",
    "left-shift-equals",
    "left shift",
    "less or equal",
    "greater",
    "right-shift-equals",
    "right shift",
    "greater or equal",
    "equal",
    "boolean equal",
    "number",
    "break",
    "case",
    "default",
    "do",
    "else",
    "for",
    "goto",
    "if",
    "return",
    "switch",
    "while",
    "identifier",
    "end of file"
};