#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../ax.h"
#include "../core.h"
#include "../tree.h"
#include "../tree/desc.h"
#include "../sexp/interp.h"
#include "../geom.h"
#include "../draw.h"
#include "../backend.h"
#include "async.h"

struct ax_state* ax_new_state()
{
    struct everything {
        struct ax_state s;
        struct ax_lexer l;
        struct ax_interp i;
        struct ax_tree t;
        struct ax_geom g;
        struct ax_async a;
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
    /* int r; */
    /* node_id root; */
    /* r = ax__build_node(s, NULL, s->tree, &AX_DESC_EMPTY_CONTAINER, &root); */
    /* ASSERT(r == 0, "build empty tree"); */

    ax__init_geom(s->geom = &e->g);

    ax__init_async(s->async = &e->a, s->geom, s->tree);

    return s;
}

void ax_destroy_state(struct ax_state* s)
{
    if (s != NULL) {
        ax__free_async(s->async);
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
    int r = ax__new_backend(s, &s->backend);
    ASSERT(r == 0, "create backend: %s", ax_get_error(s));

    ax__async_set_backend(s->async, s->backend);
    ax__async_set_dim(s->async, s->config.win_size);
}

void ax__set_error(struct ax_state* s, const char* err)
{
    free(s->err_msg);
    s->err_msg = malloc(strlen(err) + 1);
    ASSERT(s->err_msg != NULL, "malloc ax_state.err_msg");
    strcpy(s->err_msg, err);
}

int ax_wait_for_close(struct ax_state* s)
{
    ASSERT(s->backend != NULL, "backend must be initialized!");
    ax__async_wait_for_close(s->async);
    printf("bye!\n");
    return 0;
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

void ax_write_start(struct ax_state* s)
{
    ax__free_interp(s->interp);
    ax__init_interp(s->interp);
    ax__lexer_eof(s->lexer);
}

static int write_token(struct ax_state* s, enum ax_parse tok)
{
    if (s->interp->err == 0 && tok != AX_PARSE_NOTHING) {
        ax__interp(s, s->interp, s->lexer, tok);
    }
    return s->interp->err;
}

int ax_write_chunk(struct ax_state* s, const char* input)
{
    char* acc = (char*) input;
    char const* end = input + strlen(input);
    while (acc < end) {
        int r = write_token(s, ax__lexer_feed(s->lexer, acc, &acc));
        if (r != 0) {
            return r;
        }
    }
    return 0;
}

int ax_write_end(struct ax_state* s)
{
    return write_token(s, ax__lexer_eof(s->lexer));
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

void ax__set_dim(struct ax_state* s, struct ax_dim dim)
{
    ax__async_set_dim(s->async, dim);
}

void ax__set_tree(struct ax_state* s, struct ax_tree* new_tree)
{
    ax__async_set_tree(s->async, new_tree);
    ASSERT(ax__is_tree_empty(new_tree), "tree should be empty after setting");
}
