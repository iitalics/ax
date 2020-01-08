#pragma once
#include <stddef.h>

struct region_block {
    struct region_block* next;
    void* end_ptr;
    char data[0];
};

struct region_block_header {
    void* data;
    struct region_block_header* next;
};

struct region {
    char* alloc_ptr;
    char* end_ptr;

    struct region_block_header* big_blocks;
    struct region_block* used_blocks;
    struct region_block* free_blocks;
};

void ax__init_region(struct region* rgn);
void ax__free_region(struct region* rgn);

void ax__region_clear(struct region* rgn);
void* ax__region_unaligned_alloc(struct region* rgn, size_t sz);

static inline
void* ax__region_alloc(struct region* rgn, size_t sz)
{
    const size_t ALIGN_TO = 8;
    if (sz & (ALIGN_TO - 1)) {
        sz = (sz | (ALIGN_TO - 1)) + 1;
    }
    return ax__region_unaligned_alloc(rgn, sz);
}

static inline
void ax__swap_regions(struct region* fst, struct region* snd)
{
    struct region tmp = *fst;
    *fst = *snd;
    *snd = tmp;
}

void* ax__strdup(struct region* rgn, const char* str);
void* ax__strcat(struct region* rgn, const char* s1, const char* s2);

#define ALLOCATE(_rgn, T) ((T *) ax__region_alloc(_rgn, sizeof(T)))
#define ALLOCATES(_rgn, T, _n) ((T *) ax__region_alloc(_rgn, sizeof(T) * (_n)))
