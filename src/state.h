#pragma once
#include "node.h"


struct ax_state {
    struct ax_tree tree;
    struct ax_dim root_dim;
    struct ax_drawbuf* draw_buf;
};
