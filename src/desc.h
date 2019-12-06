#pragma once
#include "ax.h"


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
