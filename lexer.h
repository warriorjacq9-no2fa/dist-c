#ifndef LEXER_H
#define LEXER_H

extern const char* const token_strings[];

enum token {
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_TIMES,
    TOK_SLASH,
    TOK_PLUS,
    TOK_MINUS,
    TOK_BECOMES,
    TOK_ASSGN,
    TOK_EQUAL,
    TOK_NEQ,
    TOK_LESS,
    TOK_LEQ,
    TOK_GT,
    TOK_GEQ,
    TOK_SEMICOLON,
    TOK_COMMA,
    TOK_PERIOD,
    TOK_NUMBER,
    TOK_BEGIN,
    TOK_CALL,
    TOK_CONST,
    TOK_DO,
    TOK_END,
    TOK_IF,
    TOK_ODD,
    TOK_PROC,
    TOK_THEN,
    TOK_VAR,
    TOK_WHILE,
    TOK_IDENT,
    TOK_EOF,
};

void lex_init(char* data);

void error(const char* msg);
enum token lex_consume();
enum token lex_peek();

#endif