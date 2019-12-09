#pragma once
#include "base.h"

struct ax_lexer;
struct ax_interp;
struct ax_tree;
struct ax_geom;
struct ax_drawbuf;
struct ax_desc;

struct ax_backend_config {
    struct ax_dim win_size;
};

struct ax_state {
    char* err_msg;
    struct ax_backend_config config;

    struct ax_backend* backend;
    struct ax_lexer* lexer;
    struct ax_interp* interp;
    struct ax_tree* tree;
    struct ax_geom* geom;
    struct ax_draw_buf* draw_buf;
};

void ax__set_error(struct ax_state* s, const char* err);

void ax__initialize_backend(struct ax_state* s);

static inline
bool ax__is_backend_initialized(struct ax_state* s)
{
    return s->backend != NULL;
}

void ax__set_dim(struct ax_state* s, struct ax_dim dim);

void ax__set_tree(struct ax_state* s, struct ax_tree* tree);

static inline
void ax__config_win_size(struct ax_state* s, struct ax_dim d)
{
    s->config.win_size = d;
}
