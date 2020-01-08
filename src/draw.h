#pragma once
#include "base.h"
#include "core/region.h"
#include "core/growable.h"

/*
 * (TODO: this module should be scrapped, I think)
 */

struct ax_tree;

enum ax_draw_type {
    AX_DRAW_RECT = 0,
    AX_DRAW_TEXT,
    AX_DRAW__MAX,
};

struct ax_draw_r {
    ax_color fill;
    struct ax_aabb bounds;
};

struct ax_draw_t {
    ax_color color;
    struct ax_font* font;
    const char* text;
    struct ax_pos pos;
};

struct ax_draw {
    enum ax_draw_type ty;
    union {
        struct ax_draw_r r;
        struct ax_draw_t t;
    };
};

struct ax_draw_buf {
    struct growable growable;
};

void ax__init_draw_buf(struct ax_draw_buf* db);
void ax__free_draw_buf(struct ax_draw_buf* db);

static inline
void ax__swap_draw_bufs(struct ax_draw_buf* fst, struct ax_draw_buf* snd)
{
    struct ax_draw_buf tmp = *fst;
    *fst = *snd;
    *snd = tmp;
}

static inline
struct ax_draw* ax__draw_buf_data(struct ax_draw_buf* db)
{
    return db->growable.data;
}

static inline
const size_t ax__draw_buf_count(const struct ax_draw_buf* db)
{
    return LEN(&db->growable, struct ax_draw);
}

void ax__redraw(struct ax_tree* tr, struct ax_draw_buf* db);
