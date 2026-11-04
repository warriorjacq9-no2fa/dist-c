#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <regex.h>
#include "ast.h"

int match(const char* s, const char* restrict r) {
    regex_t regex;
    int reti;
    char msgbuf[100];

    reti = regcomp(&regex, r, REG_EXTENDED);
    if(reti) {
        regerror(reti, &regex, msgbuf, sizeof(msgbuf));
        fprintf(stderr, "Could not compile regex: %s\n", msgbuf);
        exit(1);
    }

    reti = regexec(&regex, s, 0, NULL, 0);
    if(!reti) {
        regfree(&regex);
        return 1;
    } else if(reti == REG_NOMATCH) {
        regfree(&regex);
        return 0;
    } else {
        regerror(reti, &regex, msgbuf, sizeof(msgbuf));
        fprintf(stderr, "Regex match failed: %s\n", msgbuf);
        regfree(&regex);
        return 0;
    }
}

void shift_left(AST* a, size_t n, unsigned int shift) {
    for(int i = 0; i < n - shift; i++) {
        a[i] = a[i + shift];
    }
    for(int i = n - shift; i < n; i++) {
        a[i] = (AST){0};
    }
}

enum ast_token enum_token(char* tok) {
    if(strcmp(tok, "+") == 0) return AST_SUM;
    if(strcmp(tok, "*") == 0) return AST_MUL;
    if(match(tok, "[0-9]+")) return AST_NUMBER;
    
    fprintf(stderr, "Unknown token %s\n", tok);
    exit(1);
}

int do_pass(AST* tok_list, size_t sz, enum ast_token type) {
    while(1) {
        for(int i = 0; i < sz; i++) {
            AST* tok = &tok_list[i];
            if(tok->token == type && ast_is_leaf(tok)) {
                if(i == 0) {
                    fprintf(stderr, "%s must have an lvalue\n", ast_names[type]);
                    return 1;
                }

                AST* rvalue = malloc(sizeof(AST));
                AST* lvalue = malloc(sizeof(AST));

                *rvalue = tok_list[i + 1];
                *lvalue = tok_list[i - 1];

                if(ast_is_leaf(rvalue) && rvalue->token != AST_NUMBER) {
                    fprintf(stderr, "Incorrect rvalue for %s, got %s, expected NUMBER\n", ast_names[type], ast_names[rvalue->token]);
                    return 1;
                }
                if(ast_is_leaf(lvalue) && lvalue->token != AST_NUMBER) {
                    fprintf(stderr, "Incorrect lvalue for %s, got %s, expected NUMBER\n", ast_names[type], ast_names[lvalue->token]);
                    return 1;
                }

                tok->left = lvalue;
                tok->right = rvalue;

                AST tmp = *tok;

                shift_left(&tok_list[i - 1], sz - (i - 1), 2);

                tok_list[i - 1] = tmp;
                
                i--;
            }
        }

        for(int i = 0; i < sz; i++) {
            printf("%s %d: ", ast_names[type], i);
            ast_print(&tok_list[i]);
        }

        int num = 0;
        for(int i = 0; i < sz; i++) {
            if(tok_list[i].token == type) num++;
        }

        if(num <= 1) break;
    }
    return 0;
}

AST* parse(char* data) {
    size_t sz = strlen(data);
    AST* tok_list = malloc(sz * sizeof(AST));
    memset(tok_list, 0, sz * sizeof(AST));

    // Enum and NUMBER pass
    char* tok = strtok(data, " ");
    int i = 0;
    while(tok != NULL) {
        tok_list[i].token = enum_token(tok);
        if(tok_list[i].token == AST_NUMBER) {
            tok_list[i].data.AST_NUMBER = atoi(tok);
        }
        tok = strtok(NULL, " ");
        i++;
    }

    // MUL pass
    do_pass(tok_list, sz, AST_MUL);

    // SUM pass
    do_pass(tok_list, sz, AST_SUM);

    AST* top = malloc(sizeof(AST));
    *top = tok_list[0];

    return top;
}