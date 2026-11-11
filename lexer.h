#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

extern const char* const token_strings[];

enum token {
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_LBRACE,
    TOK_RBRACE,

    TOK_LOGINV,
    TOK_COMMA,
    TOK_SEMICOLON,
    TOK_COLON,
    TOK_TERNARYIF,

    TOK_BOOLINV,
    TOK_NEQ,

    TOK_PLUS,
    TOK_EQPLUS,
    TOK_INC,

    TOK_MINUS,
    TOK_EQMINUS,
    TOK_DEC,

    TOK_STAR,
    TOK_EQMUL,

    TOK_DIV,
    TOK_EQDIV,

    TOK_MOD,
    TOK_EQMOD,

    TOK_AMPERSAND,
    TOK_BOOLAND,
    TOK_EQAND,

    TOK_XOR,
    TOK_EQXOR,

    TOK_OR,
    TOK_BOOLOR,
    TOK_EQOR,

    TOK_LESS,
    TOK_EQLSHIFT,
    TOK_LSHIFT,
    TOK_LEQ,

    TOK_GREATER,
    TOK_EQRSHIFT,
    TOK_RSHIFT,
    TOK_GEQ,

    TOK_EQUALS,
    TOK_EQUALTO,

    TOK_NUMBER,

    TOK_BREAK,
    TOK_CASE,
    TOK_DEFAULT,
    TOK_DO,
    TOK_ELSE,
    TOK_FOR,
    TOK_GOTO,
    TOK_IF,
    TOK_RETURN,
    TOK_SWITCH,
    TOK_WHILE,

    TOK_IDENT,

    TOK_EOF,
};

void lex_init(char* data);

[[noreturn]] void error(const char* msg);
enum token lex_consume();
enum token lex_peek();
void lex_back();
void lex_back_n(size_t n);

int lex_get_number();
char* lex_get_ident();

#endif