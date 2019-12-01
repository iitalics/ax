#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


typedef uint32_t ax_color;
typedef double ax_length;
typedef uint32_t ax_flex_factor;

struct ax_pos { ax_length x, y; };
struct ax_dim { ax_length w, h; };
struct ax_aabb { struct ax_pos o; struct ax_dim s; };

#define AX_POS(_x, _y) ((struct ax_pos) { .x = (_x), .y = (_y) })
#define AX_DIM(_w, _h) ((struct ax_dim) { .w = (_w), .h = (_h) })
#define AX_AABB(_x, _y, _w, _h) \
    ((struct ax_aabb) { .o = AX_POS(_x, _y), .d = AX_DIM(_w, _h) })

enum ax_node_type {
    AX_NODE_CONTAINER = 0,
    AX_NODE_RECTANGLE,
    AX_NODE_TEXT,
    AX_NODE__MAX
};

enum ax_justify {
    AX_JUSTIFY_START = 0,
    AX_JUSTIFY_END,
    AX_JUSTIFY_CENTER,
    AX_JUSTIFY_EVENLY,
    AX_JUSTIFY_AROUND,
    AX_JUSTIFY_BETWEEN,
    AX_JUSTIFY__MAX
};

struct ax_desc_c {
    const struct ax_desc* children;
    size_t n_children;
    enum ax_justify main_justify;
    enum ax_justify cross_justify;
    bool single_line;
};

struct ax_desc_r {
    ax_color fill;
    struct ax_dim size;
};

struct ax_desc_t {
    ax_color color;
    char const* text;
    void* font;
};

struct ax_flex_child_attrs {
    ax_flex_factor grow;
    ax_flex_factor shrink;
    enum ax_justify cross_justify;
};

struct ax_desc {
    enum ax_node_type ty;
    struct ax_flex_child_attrs flex_attrs;
    union {
        struct ax_desc_t t;
        struct ax_desc_c c;
        struct ax_desc_r r;
    };
};


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

struct ax_drawbuf {
    size_t len;
    size_t cap;
    struct ax_draw* data;
};

/*
 * State initialization
 */

extern struct ax_state* ax_new_state();

extern void ax_destroy_state(struct ax_state* s);

/*
 * State operations
 */

extern void ax_set_dimensions(struct ax_state* s, struct ax_dim dim);
extern struct ax_dim ax_get_dimensions(struct ax_state* s);

extern void ax_set_root(struct ax_state* s, const struct ax_desc* root);

extern const struct ax_drawbuf* ax_draw(struct ax_state* s);

/*
 * Color utilities
 */

extern void ax_color_rgb(ax_color color, uint8_t* out_rgb);

extern ax_color ax_rgb_color(uint8_t* rgb);
