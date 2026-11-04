#include <stdio.h>
#include "parse.h"
#include "ast.h"

int main(int argc, char* argv[]) {
    if(argc != 2) {
        fprintf(stderr, "Usage: %s <infix>\n", argv[0]);
        return 1;
    }

    ast_print(parse(argv[1]));
    fflush(stderr);
}