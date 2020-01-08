#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

struct growable {
    size_t size, cap;
    void* data;
};

#define DEFAULT_CAPACITY   32

void ax__init_growable(struct growable* gr, size_t cap);
void ax__free_growable(struct growable* gr);
void* ax__growable_extend(struct growable* gr, size_t len);
void* ax__growable_retract(struct growable* gr, size_t len);

static inline
bool ax__is_growable_empty(struct growable* gr)
{
    return gr->size == 0;
}

static inline
void ax__growable_clear(struct growable* gr)
{
    gr->size = 0;
}

static inline
void ax__growable_extend_with(struct growable* gr, size_t len, void* in)
{
    memcpy(ax__growable_extend(gr, len), in, len);
}

static inline
void ax__growable_retract_into(struct growable* gr, size_t len, void* out)
{
    memcpy(out, ax__growable_retract(gr, len), len);
}

static inline void ax__growable_clear_str(struct growable* gr)
{
    ((char*) gr->data)[0] = '\0';
    gr->size = 1;
}

// preserves the null byte at the end of the buffer
void ax__growable_push_str(struct growable* gr, const char* str);

static inline
void ax__growable_push_char(struct growable* gr, char c)
{
    char buf[2] = { c, '\0' };
    ax__growable_push_str(gr, buf);
}

#define PUSH(_gr, _in_ptr) ax__growable_extend_with(_gr, sizeof(*(_in_ptr)), _in_ptr)
#define POP(_gr, _out_ptr) ax__growable_retract_into(_gr, sizeof(*(_ptr)), _out_ptr)
#define LEN(_gr, type) ((_gr)->size / sizeof(type))
