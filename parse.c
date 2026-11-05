#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <regex.h>
#include "ast.h"

int do_all_passes(AST* tok_list, size_t sz);

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

void shift_left(AST* a, size_t n, size_t shift) {
    if(shift == 0 || n == 0) return;

    if(shift >= n) {
        for (size_t i = 0; i < n; i++)
            a[i] = (AST){0};
        return;
    }

    for(size_t i = 0; i < n - shift; i++)
        a[i] = a[i + shift];

    for(size_t i = n - shift; i < n; i++)
        a[i] = (AST){0};
}

int enum_tokens(AST* tok_list, char* data) {
    char* tok = strtok(data, " ");
    int i = 0, chr = 0;
    while(tok != NULL) {
        if(strcmp(tok, "(") == 0) tok_list[i].token = AST_OPEN_PAREN;
        else if(strcmp(tok, ")") == 0) tok_list[i].token = AST_CLOSE_PAREN;
        else if(strcmp(tok, "*") == 0) tok_list[i].token = AST_MUL;
        else if(strcmp(tok, "/") == 0) tok_list[i].token = AST_DIV;
        else if(strcmp(tok, "+") == 0) tok_list[i].token = AST_SUM;
        else if(strcmp(tok, "-") == 0) tok_list[i].token = AST_DIF;
        else if(match(tok, "^[0-9]+$")) {
            tok_list[i].token = AST_NUMBER;
            tok_list[i].data.AST_NUMBER = atoi(tok);
        } else {
            fprintf(stderr, "Unknown token %s at 1:%d\n", tok, chr + 1);
            return -1;
        }

        tok_list[i].col = chr;
        if(tok_list[i].token == AST_NUMBER) {
            tok_list[i].data.AST_NUMBER = atoi(tok);
        }
        chr += strlen(tok) + 1;
        tok = strtok(NULL, " ");
        i++;
    }

    return i;
}

bool contains(enum ast_token t, enum ast_token* a, size_t sz) {
    bool c = false;
    for(int i = 0; i < sz; i++) {
        if(a[i] == t) {
            c = true;
            break;
        }
    }
    return c;
}

int do_pass(AST* tok_list, size_t sz, enum ast_token* types, size_t types_sz) {
    while(1) {
        for(int i = 0; i < sz; i++) {
            AST* tok = &tok_list[i];
            if(ast_is_leaf(tok) && contains(tok->token, types, types_sz)) {
                if(i == 0) {
                    fprintf(stderr, "Expected lvalue at %d:%d\n", tok->line + 1, tok->col + 1);
                    return -1;
                }
                if(i + 1 >= sz) {
                    fprintf(stderr, "Expected rvalue at %d:%d\n", tok->line + 1, tok->col + 1);
                    return -1;
                }

                AST* rvalue = malloc(sizeof(AST));
                AST* lvalue = malloc(sizeof(AST));

                *rvalue = tok_list[i + 1];
                *lvalue = tok_list[i - 1];

                if(lvalue->token == AST_NONE) {
                    fprintf(stderr, "Expected lvalue at %d:%d\n", tok->line + 1, tok->col + 1);
                    return -1;
                }
                if(rvalue->token == AST_NONE) {
                    fprintf(stderr, "Expected rvalue at %d:%d\n", tok->line + 1, tok->col + 1);
                    return -1;
                }

                if(ast_is_leaf(rvalue) && rvalue->token != AST_NUMBER) {
                    fprintf(stderr, "Invalid rvalue: got %s, expected NUMBER at %d:%d\n",
                        ast_names[rvalue->token], tok->line + 1, tok->col + 1
                    );
                    return -1;
                }
                if(ast_is_leaf(lvalue) && lvalue->token != AST_NUMBER) {
                    fprintf(stderr, "Invalid lvalue: got %s, expected NUMBER at %d:%d\n",
                        ast_names[lvalue->token], tok->line + 1, tok->col + 1
                    );
                    return -1;
                }

                tok->left = lvalue;
                tok->right = rvalue;

                AST tmp = *tok;

                shift_left(&tok_list[i - 1], sz - (i - 1), 2);

                tok_list[i - 1] = tmp;

                i--;
            }
        }

        int num = 0;
        for(int i = 0; i < sz; i++) {
            if(contains(tok_list[i].token, types, types_sz)) num++;
            printf("%d: ", i);
            ast_print(&tok_list[i]);
        }

        if(num <= 1) break;
    }
    return 0;
}

int do_grouping_pass(AST* tok_list, size_t sz, enum ast_token* starts, size_t starts_sz, enum ast_token* ends, size_t ends_sz) {
    while(1) {
        for(int i = 0; i < sz; i++) {
            AST* tok = &tok_list[i];
            if(ast_is_leaf(tok) && contains(tok->token, starts, starts_sz)) {
                printf("Found grouping start at %d\n", i);
                if(i + 1 >= sz) {
                    fprintf(stderr, "Grouping symbols empty at %d:%d\n", tok->line + 1, tok->col + 1);
                    return -1;
                }

                int end = -1;
                int c = 1;
                for(int j = i + 1; j < sz; j++) {
                    if(ast_is_leaf(&tok_list[j]) && contains(tok_list[j].token, ends, ends_sz)) {
                        c--;
                        printf("Found grouping end at %d (%d)\n", j, c);
                        if(c == 0) {
                            end = j;
                            printf("End at %d\n", end);
                            break;
                        }
                    } else if(ast_is_leaf(&tok_list[j]) && contains(tok_list[j].token, starts, starts_sz)) {
                        c++;
                        printf("Found grouping start at %d (%d)\n", j, c);
                    }
                }

                if(end == -1) {
                    fprintf(stderr, "Grouping symbol start without end at %d:%d\n", tok->line + 1, tok->col + 1);
                    return -1;
                }

                tok_list[end] = (AST){0};

                printf("Doing passes from %d for %d\n", i + 1, end - i - 1);

                if(do_all_passes(&tok_list[i + 1], end - i - 1) < 0) return -1;
                printf("Done\n");

                AST tmp = tok_list[i + 1];

                shift_left(&tok_list[i], sz - i, end - i);

                *tok = tmp;

                for(int j = 0; j < sz; j++) {
                    printf("%d: ", j);
                    ast_print(&tok_list[j]);
                }

                i--;
            } else if(ast_is_leaf(tok) && contains(tok->token, ends, ends_sz)) {
                fprintf(stderr, "Unexpected grouping symbol end at %d:%d\n", tok->line + 1, tok->col + 1);
                return -1;
            }
        }

        int num = 0;
        for(int i = 0; i < sz; i++) {
            if(
                contains(tok_list[i].token, starts, starts_sz) ||
                contains(tok_list[i].token, ends, ends_sz)
            ) num++;
            printf("%d: ", i);
            ast_print(&tok_list[i]);
        }

        if(num <= 1) break;
    }
    return 0;
}

int do_all_passes(AST* tok_list, size_t sz) {
    for(int j = 0; j < sz; j++) {
        printf("%d: ", j);
        ast_print(&tok_list[j]);
    }
    // Parentheses
    printf("Parens (\n");
    if(do_grouping_pass(tok_list, sz,
        (enum ast_token[]){AST_OPEN_PAREN}, 1, (enum ast_token[]){AST_CLOSE_PAREN}, 1
    ) < 0) return -1;
    printf("Parens )\n");

    // MUL/DIV
    printf("MUL/DIV\n");
    if(do_pass(tok_list, sz, (enum ast_token[]){AST_MUL, AST_DIV}, 2) < 0) return -1;

    // SUM/DIF
    printf("SUM/DIF\n");
    if(do_pass(tok_list, sz, (enum ast_token[]){AST_SUM, AST_DIF}, 2) < 0) return -1;

    return 0;
}

AST* parse(char* data) {
    size_t sz = strlen(data);
    AST* tok_list = malloc(sz * sizeof(AST));
    memset(tok_list, 0, sz * sizeof(AST));

    // Enum and NUMBER pass
    int ret = enum_tokens(tok_list, data);
    if(ret < 0) return NULL;
    sz = (size_t) ret;

    if(do_all_passes(tok_list, sz) < 0) return NULL;

    AST* top = malloc(sizeof(AST));
    *top = tok_list[0];

    return top;
}