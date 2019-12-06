#pragma once
#include "../base.h"
#include "../tree.h"

typedef uint32_t ax_flex_factor;

struct ax_desc_c {
    const struct ax_desc* first_child;
    enum ax_justify main_justify;
    enum ax_justify cross_justify;
    bool single_line;
    ax_color background;
};

struct ax_desc_t {
    ax_color color;
    const char* text;
    const char* font_name;
};

struct ax_flex_child_attrs {
    ax_flex_factor grow;
    ax_flex_factor shrink;
    enum ax_justify cross_justify;

    struct ax_desc* next_child;
};

struct ax_desc {
    enum ax_node_type ty;
    struct ax_desc* parent;
    struct ax_flex_child_attrs flex_attrs;
    union {
        struct ax_desc_t t;
        struct ax_desc_c c;
        struct ax_rect r;
    };
};

#define AX_DESC_EMPTY_CONTAINER ((struct ax_desc)         \
    { .ty = AX_NODE_CONTAINER,                            \
      .parent = NULL,                                     \
      .flex_attrs = { .grow = 0, .shrink = 1,             \
                      .cross_justify = AX_JUSTIFY_START,  \
                      .next_child = NULL },               \
      .c = { .first_child = NULL,                         \
             .main_justify = AX_JUSTIFY_START,            \
             .cross_justify = AX_JUSTIFY_START } })

static inline
struct ax_desc* ax_desc_reverse_flex_children(struct ax_desc* init_desc)
{
    struct ax_desc* first, *last, *next;
    for (first = NULL, last = init_desc; last != NULL; last = next)
    {
        next = last->flex_attrs.next_child;
        last->flex_attrs.next_child = first;
        first = last;
    }
    return first;
}
