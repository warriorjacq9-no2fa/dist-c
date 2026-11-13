#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "dict.h"

struct dict* dict_new(size_t sz) {
    struct dict* dict = malloc(sizeof(struct dict));
    dict->k = malloc(sizeof(struct array) + sz * sizeof(void*));
    dict->k->sz = sz;
    dict->v = malloc(sizeof(struct array) + sz * sizeof(void*));
    dict->v->sz = sz;
    return dict;
}

void dict_free(struct dict* dict) {
    for(size_t i = 0; i < dict->k->sz; i++) free(dict->k->data[i]);
    for(size_t i = 0; i < dict->v->sz; i++) free(dict->v->data[i]);

    free(dict->k);
    free(dict->v);
    free(dict);
}

/* Append src onto dict and destroy src */
void dict_append(struct dict* dest, struct dict* src) {
    size_t k_sz = sizeof(struct array) + (dest->k->sz + src->k->sz) * sizeof(void*);
    size_t v_sz = sizeof(struct array) + (dest->v->sz + src->v->sz) * sizeof(void*);

    dest->k = realloc(dest->k, k_sz);
    dest->v = realloc(dest->v, v_sz);

    memcpy(&dest->k->data[dest->k->sz], src->k->data, src->k->sz * sizeof(void*));
    dest->k->sz += src->k->sz;

    memcpy(&dest->v->data[dest->v->sz], src->v->data, src->v->sz * sizeof(void*));
    dest->v->sz += src->v->sz;

    free(src->k);
    free(src->v);
    free(src);
}

void* dict_get(struct dict* dict, char* key) {
    for(size_t i = 0; i < dict->k->sz; i++) {
        if(strcmp((char*)dict->k->data[i], key) == 0)
            return dict->v->data[i];
    }
    return NULL;
}