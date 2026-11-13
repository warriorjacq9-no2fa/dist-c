#ifndef DICT_H
#define DICT_H

#include <stddef.h>

struct array {
    size_t sz;
    void* data[];
};

struct dict {
    struct array* k;
    struct array* v;
};

struct dict* dict_new(size_t sz);
void dict_free(struct dict* dict);
void dict_append(struct dict* dest, struct dict* src);
void* dict_get(struct dict* dict, char* key);
void dict_print(struct dict* dict);

#endif