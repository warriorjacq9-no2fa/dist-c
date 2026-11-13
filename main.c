#include <stdlib.h>
#include <stdio.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "compiler.h"

int main(int argc, char* argv[]) {
    if(argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    FILE* fp = fopen(argv[1], "r");
    if(fp == NULL) {
        perror("Failed to open file");
        return 1;
    }
    if(fseek(fp, 0, SEEK_END) != 0) {
        perror("Seek failed");
        fclose(fp);
        return 1;
    }
    size_t size = ftell(fp);

    if(fseek(fp, 0, SEEK_SET) != 0) {
        perror("Seek failed");
        fclose(fp);
        return 1;
    }

    char* data = malloc(size);

    if(fread(data, 1, size, fp) != size) {
        perror("Read failed");
        fclose(fp);
        return 1;
    }
    fclose(fp);

    lex_init(data);
    struct ast* ast = program();
    ast_print(ast);
    compile(ast);
    return 0;
}