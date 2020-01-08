#include <stdlib.h>
#include <string.h>
#include "region.h"
#include "../utils.h"

#define BLOCK_SIZE       4096
#define MAX_ALLOC_SIZE   (BLOCK_SIZE - sizeof(struct region_block))

static inline
struct region_block* new_region_block(void)
{
    struct region_block* blk = malloc(BLOCK_SIZE);
    ASSERT(blk != NULL, "malloc new region block");
    blk->end_ptr = (char*) blk + BLOCK_SIZE;
    return blk;
}

static inline
void destroy_region_blocks(struct region_block* blk)
{
    while (blk != NULL) {
        struct region_block* next = blk->next;
        free(blk);
        blk = next;
    }
}

static inline
void* new_big_block(struct region* rgn, size_t sz)
{
    void* data = malloc(sz);
    ASSERT(data != NULL, "malloc big block data");
    struct region_block_header* hdr = ALLOCATE(rgn, struct region_block_header);
    hdr->data = data;
    hdr->next = rgn->big_blocks;
    rgn->big_blocks = hdr;
    return data;
}

static inline
void destroy_big_blocks(struct region* rgn)
{
    struct region_block_header* hdr = rgn->big_blocks;
    while (hdr != NULL) {
        free(hdr->data);
        hdr = hdr->next;
    }
    rgn->big_blocks = NULL;
}

static inline
void use_fresh_block(struct region* rgn)
{
    struct region_block* fresh;
    if (rgn->free_blocks != NULL) {
        fresh = rgn->free_blocks;
        rgn->free_blocks = fresh->next;
    } else {
        fresh = new_region_block();
    }
    fresh->next = rgn->used_blocks;
    rgn->used_blocks = fresh;
    rgn->alloc_ptr = fresh->data;
    rgn->end_ptr = fresh->end_ptr;
}

void ax__init_region(struct region* rgn)
{
    rgn->big_blocks = NULL;
    rgn->free_blocks = NULL;
    rgn->used_blocks = NULL;
    use_fresh_block(rgn);
}

void ax__free_region(struct region* rgn)
{
    destroy_big_blocks(rgn);
    destroy_region_blocks(rgn->used_blocks);
    destroy_region_blocks(rgn->free_blocks);
}

void ax__region_clear(struct region* rgn)
{
    struct region_block* ubs = rgn->used_blocks;
    struct region_block* fbs = rgn->free_blocks;
    while (ubs != NULL) {
        struct region_block* next = ubs->next;
        ubs->next = fbs;
        fbs = ubs;
        ubs = next;
    }
    destroy_big_blocks(rgn);
    rgn->used_blocks = NULL;
    rgn->free_blocks = fbs;
    use_fresh_block(rgn);
}

void* ax__region_unaligned_alloc(struct region* rgn, size_t sz)
{
    if (sz >= MAX_ALLOC_SIZE) {
        return new_big_block(rgn, sz);
    }
    if (rgn->alloc_ptr + sz > rgn->end_ptr) {
        use_fresh_block(rgn);
    }
    void* ptr = rgn->alloc_ptr;
    rgn->alloc_ptr += sz;
    return ptr;
}

void* ax__strdup(struct region* rgn, const char* str)
{
    size_t sz = strlen(str) + 1;
    char* new = ALLOCATES(rgn, char, sz);
    strcpy(new, str);
    return new;
}

void* ax__strcat(struct region* rgn, const char* s1, const char* s2)
{
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    char* new = ALLOCATES(rgn, char, len1 + len2 + 1);
    strcpy(new, s1);
    strcpy(new + len1, s2);
    return new;
}
