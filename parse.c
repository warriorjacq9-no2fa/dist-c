#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include "ast.h"

int match(const char* s, const char* restrict r) {
    regex_t regex;
    int reti;
    char msgbuf[100];

    reti = regcomp(&regex, r, 0);
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

enum ast_tag enum_token(char* tok) {
    if(strcmp(tok, "+")) return AST_ADD;
    if(strcmp(tok, "*")) return AST_MUL;
    if(match(tok, "[0-9]+")) return AST_NUMBER;
    fprintf(stderr, "Unkown token %s\n", tok);
    exit(1);
}

AST* parse(char* data) {
    char* saveptr;
    char* tok = strtok_r(data, " ", &saveptr);
    while(tok != NULL) {
        AST ast = (AST) {.tag = enum_token(tok)};
        switch(ast.tag) {
            case AST_NUMBER: {
                struct AST_NUMBER* data = &ast.data.AST_NUMBER;
                data->number = atoi(tok);
                break;
            }
            case AST_ADD: {
                struct AST_ADD* data = &ast.data.AST_ADD;
            }
        }
        tok = strtok(NULL, " ");
    }
}