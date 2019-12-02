#pragma once
#include "node.h"
#include "sexp.h"


struct ax_interp {
    int state;
    int mode;
    char* err_msg;

    union {
        struct ax_dim dim;
    };
};

struct ax_state {
    struct ax_tree tree;
    struct ax_dim root_dim;
    struct ax_drawbuf* draw_buf;
    struct ax_interp interp;
    struct ax_parser parser;
};
