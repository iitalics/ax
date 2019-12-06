#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef uint32_t ax_color;
typedef double ax_length;

struct ax_pos { ax_length x, y; };
struct ax_dim { ax_length w, h; };
struct ax_aabb { struct ax_pos o; struct ax_dim s; };

#define AX_NULL_COLOR ((ax_color) -1)
#define AX_COLOR_IS_NULL(_c) ((_c) >= 0x1000000)

#define AX_POS(_x, _y) ((struct ax_pos) { .x = (_x), .y = (_y) })
#define AX_DIM(_w, _h) ((struct ax_dim) { .w = (_w), .h = (_h) })
#define AX_AABB(_x, _y, _w, _h) \
    ((struct ax_aabb) { .o = AX_POS(_x, _y), .d = AX_DIM(_w, _h) })

extern bool ax_color_to_rgb(ax_color color, uint8_t* out_rgb);

extern ax_color ax_color_from_rgb(uint8_t* rgb);


/* Lost and found */

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
