#pragma once
#include "base.h"

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
    void* font;
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
    size_t len;
    size_t cap;
    struct ax_draw* data;
};

void ax__init_draw_buf(struct ax_draw_buf* db);
void ax__free_draw_buf(struct ax_draw_buf* db);

void ax__redraw(struct ax_tree* tr, struct ax_draw_buf* db);
