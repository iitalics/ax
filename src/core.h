#pragma once
#include "base.h"

struct ax_lexer;
struct ax_interp;
struct ax_tree;
struct ax_geom;
struct ax_drawbuf;
struct ax_desc;

struct ax_state {
    char* err_msg;
    struct ax_lexer* lexer;
    struct ax_interp* interp;
    struct ax_tree* tree;
    struct ax_geom* geom;
    struct ax_draw_buf* draw_buf;
};

void ax__set_error(struct ax_state* s, const char* err);
void ax__set_dim(struct ax_state* s, struct ax_dim dim);
void ax__set_root(struct ax_state* s, const struct ax_desc* root);
