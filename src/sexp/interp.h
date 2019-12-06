#pragma once
#include "../base.h"
#include "../sexp.h"

struct ax_state;
struct ax_desc;

struct ax_interp {
    char* err_msg;

    // for sexp parser
    int state;
    int ctx;
    int ctx_stack[128];
    size_t ctx_sp;

    // for building nodes
    struct ax_desc* desc;
    struct ax_desc* parent_desc;

    // for parsing primitive types
    int mode;
    union {
        struct ax_dim dim;
        struct {
            uint8_t rgb[3];
            size_t rgb_idx;
        } col;
    };
};

void ax__init_interp(struct ax_interp* it);
void ax__free_interp(struct ax_interp* it);

int ax__interp(struct ax_state* s,
               struct ax_interp* it,
               struct ax_parser* pr, enum ax_parse p);
