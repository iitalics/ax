#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
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
    struct region rgn;
    ax__init_region(&rgn);

    struct ax_state* s = ALLOCATE(&rgn, struct ax_state);

    s->config = (struct ax_backend_config) {
        .win_size = AX_DIM(800, 600)
    };

    ax__init_region(&s->err_msg_rgn);
    s->err_msg = NULL;

    int evt_fds[2];
    int rv = pipe(evt_fds);
    ASSERT(rv == 0, "pipe creation failed: %s", strerror(errno));
    s->evt_read_fd = evt_fds[0];
    s->evt_write_fd = evt_fds[1];

    s->backend = NULL;

    ax__init_lexer(s->lexer = ALLOCATE(&rgn, struct ax_lexer));
    ax__init_interp(s->interp = ALLOCATE(&rgn, struct ax_interp));
    ax__init_tree(s->tree = ALLOCATE(&rgn, struct ax_tree));
    ax__init_geom(s->geom = ALLOCATE(&rgn, struct ax_geom));
    ax__init_async(s->async = ALLOCATE(&rgn, struct ax_async),
                   s->geom,
                   s->tree,
                   s->evt_write_fd);

    s->init_rgn = rgn;
    return s;
}

void ax_destroy_state(struct ax_state* s)
{
    if (s != NULL) {
        // NOTE: closing evt_write_fd before shutting down the async subsystem is an easy
        //       way to unblock the "evt thread" in case it's blocked for some reason.
        //       however i think this causes it to spit out a "broken pipe" error.
        close(s->evt_write_fd);
        close(s->evt_read_fd);
        ax__free_async(s->async);
        ax__free_geom(s->geom);
        ax__free_tree(s->tree);
        ax__free_interp(s->interp);
        ax__free_lexer(s->lexer);
        ax__destroy_backend(s->backend);
        ax__free_region(&s->err_msg_rgn);

        struct region init_rgn = s->init_rgn;
        ax__free_region(&init_rgn);
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
    ax__region_clear(&s->err_msg_rgn);
    s->err_msg = ax__strdup(&s->err_msg_rgn, err);
}

int ax_poll_event_fd(struct ax_state* s)
{
    return s->evt_read_fd;
}

bool ax_poll_event(struct ax_state* s)
{
    int fd = ax_poll_event_fd(s);
    fd_set rd_fds;
    FD_ZERO(&rd_fds);
    FD_SET(fd, &rd_fds);
    struct timeval tmout;
    tmout.tv_sec = tmout.tv_usec = 0;
    return select(fd + 1, &rd_fds, NULL, NULL, &tmout) > 0;
}

void ax_read_close_event(struct ax_state* s)
{
    ASSERT(s->backend != NULL, "backend must be initialized!");

    char buf[2] = {'\0', '\0'};
    size_t n = read(ax_poll_event_fd(s), buf, 1);
    if (n > 0) {
        ASSERT(strcmp(buf, "C") == 0, "got \"%s\"", buf);
    }
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
    // TODO: find better way to reset an interp
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

int ax_write_chunk(struct ax_state* s, const char* input, size_t len)
{
    char* acc = (char*) input;
    char const* end = input + len;
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
    if ((r = ax_write_chunk(s, input, strlen(input))) != 0) {
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
