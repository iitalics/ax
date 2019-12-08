#include <stdio.h>
#include <string.h>

#include "../core.h"
#include "../tree.h"
#include "../tree/desc.h"
#include "../sexp/interp.h"
#include "../geom.h"
#include "../draw.h"
#include "../backend.h"

struct ax_state* ax_new_state()
{
    struct everything {
        struct ax_state s;
        struct ax_lexer l;
        struct ax_interp i;
        struct ax_tree t;
        struct ax_geom g;
        struct ax_draw_buf d;
    };

    struct everything* e = malloc(sizeof(struct everything));
    ASSERT(e != NULL, "malloc ax_state");

    struct ax_state* s = &e->s;

    s->err_msg = NULL;
    s->config = (struct ax_backend_config) {
        .win_size = AX_DIM(800, 600),
    };
    s->backend = NULL;

    ax__init_lexer(s->lexer = &e->l);
    ax__init_interp(s->interp = &e->i);
    ax__init_tree(s->tree = &e->t);
    ax__build_node(s, s->tree, &AX_DESC_EMPTY_CONTAINER);
    ax__init_geom(s->geom = &e->g);
    ax__init_draw_buf(s->draw_buf = &e->d);
    return s;
}

void ax_destroy_state(struct ax_state* s)
{
    if (s != NULL) {
        ax__free_draw_buf(s->draw_buf);
        ax__free_geom(s->geom);
        ax__free_tree(s->tree);
        ax__free_interp(s->interp);
        ax__free_lexer(s->lexer);
        ax__destroy_backend(s->backend);
        free(s->err_msg);
        free(s);
    }
}

void ax__initialize_backend(struct ax_state* s)
{
    ASSERT(s->backend == NULL, "backend already initialized");
    s->backend = ax__create_backend(s);
    s->geom->root_dim = s->config.win_size;
}

void ax__set_error(struct ax_state* s, const char* err)
{
    free(s->err_msg);
    s->err_msg = malloc(strlen(err) + 1);
    ASSERT(s->err_msg != NULL, "malloc ax_state.err_msg");
    strcpy(s->err_msg, err);
}

int ax_event_loop(struct ax_state* s)
{
    return ax__event_loop(s);
}

const char* ax_get_error(struct ax_state* s)
{
    if (s->err_msg != NULL) {
        return s->err_msg;
    } if (s->interp->err_msg != NULL) {
        return s->interp->err_msg;
    } else {
        return NULL;
    }
}

const struct ax_draw_buf* ax_draw(struct ax_state* s)
{
    return s->draw_buf;
}

void ax_write_start(struct ax_state* s)
{
    ax__free_interp(s->interp);
    ax__init_interp(s->interp);
    ax__lexer_eof(s->lexer);
}

int ax_write_chunk(struct ax_state* s, const char* input)
{
    char* acc = (char*) input;
    char const* end = input + strlen(input);
    while (acc < end) {
        enum ax_parse p = ax__lexer_feed(s->lexer, acc, &acc);
        if (p != AX_PARSE_NOTHING) {
            int r = ax__interp(s, s->interp, s->lexer, p);
            if (r != 0) {
                return r;
            }
        }
    }
    return 0;
}

int ax_write_end(struct ax_state* s)
{
    enum ax_parse p = ax__lexer_eof(s->lexer);
    return ax__interp(s, s->interp, s->lexer, p);
}

int ax_write(struct ax_state* s, const char* input)
{
    ax_write_start(s);
    int r;
    if ((r = ax_write_chunk(s, input)) != 0) {
        return r;
    }
    return ax_write_end(s);
}

static void invalidate(struct ax_state* s)
{
    ax__layout(s->tree, s->geom);
    ax__redraw(s->tree, s->draw_buf);
}

void ax__set_dim(struct ax_state* s, struct ax_dim dim)
{
    s->geom->root_dim = dim;
    invalidate(s);
}

void ax__set_root(struct ax_state* s, const struct ax_desc* root)
{
    // TODO: make this an error code, not an assert
    ASSERT(s->backend != NULL, "backend must be initialized");

    ax__tree_clear(s->tree);
    node_id r = ax__build_node(s, s->tree, root);
    ASSERT(ax__node_by_id(s->tree, r) == ax__root(s->tree), "init node should be root");
    invalidate(s);
}
