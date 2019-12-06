#include <stdio.h>
#include <string.h>

#define AX_DEFINE_TRAVERSAL_MACROS 1
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
        struct ax_parser p;
        struct ax_interp i;
        struct ax_tree t;
        struct ax_geom g;
        struct ax_draw_buf d;
    };

    struct everything* e = malloc(sizeof(struct everything));
    ASSERT(e != NULL, "malloc ax_state");

    struct ax_state* s = &e->s;
    ax__init_parser(s->parser = &e->p);
    ax__init_interp(s->interp = &e->i);
    ax__init_tree(s->tree = &e->t);
    ax__init_geom(s->geom = &e->g);
    ax__init_draw_buf(s->draw_buf = &e->d);
    ax__set_root(s, &AX_DESC_EMPTY_CONTAINER);
    return s;
}

void ax_destroy_state(struct ax_state* s)
{
    if (s != NULL) {
        ax__free_draw_buf(s->draw_buf);
        ax__free_geom(s->geom);
        ax__free_tree(s->tree);
        ax__free_interp(s->interp);
        ax__free_parser(s->parser);
        free(s);
    }
}

const char* ax_get_error(struct ax_state* s)
{
    if (s->interp->err_msg != NULL) {
        return s->interp->err_msg;
    } else {
        return "(no error)";
    }
}

const struct ax_draw_buf* ax_draw(struct ax_state* s)
{
    return s->draw_buf;
}

void ax_read_start(struct ax_state* s)
{
    ax__init_interp(s->interp);
    ax__free_interp(s->interp);
    ax__parser_eof(s->parser);
}

int ax_read_chunk(struct ax_state* s, const char* input)
{
    char* acc = (char*) input;
    char const* end = input + strlen(input);
    while (acc < end) {
        enum ax_parse p = ax__parser_feed(s->parser, acc, &acc);
        if (p != AX_PARSE_NOTHING) {
            int r = ax__interp(s, s->interp, s->parser, p);
            if (r != 0) {
                return r;
            }
        }
    }
    return 0;
}

int ax_read_end(struct ax_state* s)
{
    enum ax_parse p = ax__parser_eof(s->parser);
    return ax__interp(s, s->interp, s->parser, p);
}

void ax__invalidate(struct ax_state* s)
{
    ax__layout(s->tree, s->geom);
    ax__redraw(s->tree, s->draw_buf);
}

void ax__set_dim(struct ax_state* s, struct ax_dim dim)
{
    s->geom->root_dim = dim;
    ax__invalidate(s);
}

void ax__set_root(struct ax_state* s, const struct ax_desc* root)
{
    ax__tree_clear(s->tree);
    node_id r = ax__build_node(s->tree, root);
    ASSERT(ax__node_by_id(s->tree, r) == ax__root(s->tree), "init node should be root");
    ax__invalidate(s);
}
