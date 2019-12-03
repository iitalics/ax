#pragma once
#include "node.h"
#include "sexp.h"


struct ax_interp {
    // used by sexp parser
    int state;
    int ctx;
    int ctx_stack[128];
    size_t ctx_sp;

    int mode;
    char* err_msg;

    union {
        struct ax_dim dim;
    };

    struct ax_desc* desc;
    struct ax_desc* parent_desc;
};

struct ax_state {
    struct ax_tree tree;
    struct ax_dim root_dim;
    struct ax_drawbuf* draw_buf;
    struct ax_interp interp;
    struct ax_parser parser;
};
