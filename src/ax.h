#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * Common core types
 */

typedef uint32_t ax_color;
typedef double ax_length;
typedef uint32_t ax_flex_factor;

struct ax_pos { ax_length x, y; };
struct ax_dim { ax_length w, h; };
struct ax_aabb { struct ax_pos o; struct ax_dim s; };

/*
 * Common node util types
 */

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

struct ax_rect {
    ax_color fill;
    struct ax_dim size;
};

/*
 * Drawing types
 */

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

extern const struct ax_drawbuf* ax_draw(struct ax_state* s);
extern const char* ax_get_error(struct ax_state* s);

/*
 * String based API
 */

extern void ax_read_start(struct ax_state* s);
extern int ax_read_chunk(struct ax_state* s, const char* input);
extern int ax_read_end(struct ax_state* s);

static inline int ax_read(struct ax_state* s, const char* input)
{
    ax_read_start(s);
    int r;
    if ((r = ax_read_chunk(s, input)) != 0) {
        return r;
    }
    return ax_read_end(s);
}

/*
 * Other utilities
 */

#define AX_NULL_COLOR ((ax_color) -1)
#define AX_COLOR_IS_NULL(_c) ((_c) >= 0x1000000)

#define AX_POS(_x, _y) ((struct ax_pos) { .x = (_x), .y = (_y) })
#define AX_DIM(_w, _h) ((struct ax_dim) { .w = (_w), .h = (_h) })
#define AX_AABB(_x, _y, _w, _h) \
    ((struct ax_aabb) { .o = AX_POS(_x, _y), .d = AX_DIM(_w, _h) })

// returns false if color is invalid
extern bool ax_color_rgb(ax_color color, uint8_t* out_rgb);

// returns AX_NULL_COLOR if rgb is NULL
extern ax_color ax_rgb_color(uint8_t* rgb);
