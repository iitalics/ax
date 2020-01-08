#include <stdlib.h>
#include "growable.h"
#include "../utils.h"

void ax__init_growable(struct growable* gr, size_t cap)
{
    gr->size = 0;
    gr->cap = cap;
    gr->data = malloc(cap);
}

void ax__free_growable(struct growable* gr)
{
    free(gr->data);
}

void* ax__growable_extend(struct growable* gr, size_t len)
{
    while (gr->size + len > gr->cap) {
        gr->cap *= 2;
        gr->data = realloc(gr->data, gr->cap);
    }

    void* top = ((char*) gr->data) + gr->size;
    gr->size += len;
    return top;
}

void* ax__growable_retract(struct growable* gr, size_t len)
{
    ASSERT(gr->size >= len, "attempted to pop too many bytes, %zu left", gr->size);
    gr->size -= len;
    return ((char*) gr->data) + gr->size;
}

void ax__growable_push_str(struct growable* gr, const char* str)
{
    if (gr->size == 0) {
        gr->size = 1;
    }

    char* eos = ax__growable_extend(gr, strlen(str));
    strcpy(eos - 1, str);
}
