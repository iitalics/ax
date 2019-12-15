#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "async.h"
#include "../backend.h"
#include "../core.h"
#include "../tree.h"
#include "../geom.h"
#include "../utils.h"

#define LOCK(res, _body) do {                   \
        pthread_mutex_lock(&res ## _mx);        \
        _body;                                  \
        pthread_mutex_unlock(&res ## _mx);      \
    } while (0)

#define SEND(sys, _msg, _setup)                     \
    LOCK(sys.msg, {                                 \
            sys.msg |= (_msg);                      \
            _setup;                                 \
            pthread_cond_signal(&sys.new_msg_cv);   \
        })

#define SEND_SYNC(sys, cv, _msg, _setup) do {   \
        SEND(sys, _msg, {                       \
                _setup;                         \
                pthread_mutex_lock(&cv ## _mx); \
            });                                 \
        pthread_cond_wait(&cv, &cv ## _mx);     \
        pthread_mutex_unlock(&cv ## _mx);       \
    } while (0)

#define NOTIFY(cv) LOCK(cv, pthread_cond_broadcast(&cv))

#define RECV(sys, _msg, _wait, _body)               \
    LOCK(sys.msg, {                                 \
            if (sys.msg == 0 && (_wait)) {          \
                pthread_cond_wait(&sys.new_msg_cv,  \
                                  &sys.msg_mx);     \
            }                                       \
            _msg = sys.msg;                         \
            sys.msg = 0;                            \
            _body;                                  \
        });

#define JOIN(sys) do {                                          \
        void* _ret;                                             \
        ASSERT(pthread_join(sys.thd, &_ret) == 0,               \
               "%s", strerror(errno));                          \
        ASSERT(_ret == &sys, "thread didn't exit properly");    \
    } while (0)

static void* layout_thd(void*);
static void* ui_thd(void*);

void ax__init_async(struct ax_state* s, struct ax_async* async)
{
    async->geom = s->geom;
    async->tree = s->tree;
    ax__init_draw_buf(&async->layout.db);
    ax__init_draw_buf(&async->ui.display_db);
    ax__init_draw_buf(&async->ui.scratch_db);

    pthread_mutex_init(&async->layout.msg_mx, NULL);
    pthread_mutex_init(&async->layout.tree_mx, NULL);
    pthread_mutex_init(&async->layout.on_tree_modify_mx, NULL);
    pthread_mutex_init(&async->layout.on_layout_mx, NULL);
    pthread_cond_init(&async->layout.new_msg_cv, NULL);
    pthread_cond_init(&async->layout.on_tree_modify, NULL);
    pthread_cond_init(&async->layout.on_layout, NULL);

    pthread_mutex_init(&async->ui.msg_mx, NULL);
    pthread_mutex_init(&async->ui.scratch_db_mx, NULL);
    pthread_mutex_init(&async->ui.on_close_mx, NULL);
    pthread_cond_init(&async->ui.new_msg_cv, NULL);
    pthread_cond_init(&async->ui.on_close, NULL);

    pthread_create(&async->layout.thd, NULL, layout_thd, (void*) async);
    pthread_create(&async->ui.thd, NULL, ui_thd, (void*) async);
}

void ax__free_async(struct ax_async* async)
{
    SEND(async->layout, ASYNC_QUIT, {});
    SEND(async->ui, ASYNC_QUIT, {});

    JOIN(async->layout);
    JOIN(async->ui);

    pthread_cond_destroy(&async->ui.on_close);
    pthread_cond_destroy(&async->ui.new_msg_cv);
    pthread_mutex_destroy(&async->ui.on_close_mx);
    pthread_mutex_destroy(&async->ui.scratch_db_mx);
    pthread_mutex_destroy(&async->ui.msg_mx);

    pthread_cond_destroy(&async->layout.on_layout);
    pthread_cond_destroy(&async->layout.on_tree_modify);
    pthread_cond_destroy(&async->layout.new_msg_cv);
    pthread_mutex_destroy(&async->layout.on_layout_mx);
    pthread_mutex_destroy(&async->layout.on_tree_modify_mx);
    pthread_mutex_destroy(&async->layout.tree_mx);
    pthread_mutex_destroy(&async->layout.msg_mx);

    ax__free_draw_buf(&async->ui.scratch_db);
    ax__free_draw_buf(&async->ui.display_db);
    ax__free_draw_buf(&async->layout.db);
}

static void layout_thd_handle(struct ax_async* async, int msg,
                              bool* out_quit, bool* out_needs_layout,
                              bool* out_notify_about_layout)
{
    if (msg & ASYNC_QUIT) {
        *out_quit = true;
    }
    if (msg & ASYNC_SET_DIM) {
        *out_needs_layout = true;
        async->geom->root_dim = async->layout.dim;
    }
    if (msg & ASYNC_SET_TREE) {
        *out_needs_layout = true;
        ax__tree_drain_from(async->tree, async->layout.tree);
        NOTIFY(async->layout.on_tree_modify);
    }
    if (msg & ASYNC_WAIT_FOR_LAYOUT) {
        *out_notify_about_layout = true;
    }
}

static void* layout_thd(void* ud)
{
    struct ax_async* async = ud;

    for (bool quit = false; !quit; ) {
        bool needs_layout = false;
        bool notify_about_layout = false;
        int msg;
        RECV(async->layout, msg, true,
             layout_thd_handle(async, msg, &quit, &needs_layout, &notify_about_layout));

        if (needs_layout) {
            ax__layout(async->tree, async->geom);
            ax__redraw(async->tree, &async->layout.db);
            SEND(async->ui, ASYNC_FLIP_BUFFERS,
                 LOCK(async->ui.scratch_db,
                      ax__swap_draw_bufs(&async->layout.db,
                                         &async->ui.scratch_db)));
        }

        if (notify_about_layout) {
            NOTIFY(async->layout.on_layout);
        }
    }

    return &async->layout;
}

static void ui_thd_handle(struct ax_async* async, int msg,
                          bool* out_quit, struct ax_backend** out_bac)
{
    if (msg & ASYNC_QUIT) {
        *out_quit = true;
    }
    if (msg & ASYNC_SET_BACKEND) {
        *out_bac = async->ui.backend;
    }
    if (msg & ASYNC_FLIP_BUFFERS) {
        LOCK(async->ui.scratch_db,
             ax__swap_draw_bufs(&async->ui.display_db, &async->ui.scratch_db));
    }
}

static void* ui_thd(void* ud)
{
    struct ax_async* async = ud;
    struct ax_backend* bac = NULL;
    bool quit = false;
    bool closed = false;
    while (!quit) {
        bool backend_enabled = bac != NULL && !closed;
        if (backend_enabled) {
            ax__poll_events(bac, &closed);
            ax__render(bac,
                       async->ui.display_db.data,
                       async->ui.display_db.len);
            ax__wait_for_frame(bac);
        }

        int msg;
        RECV(async->ui, msg, !backend_enabled,
             ui_thd_handle(async, msg, &quit, &bac));

        if (closed) {
            NOTIFY(async->ui.on_close);
        }
    }

    return &async->ui;
}

void ax__async_set_dim(struct ax_async* async, struct ax_dim dim)
{
    SEND(async->layout,
         ASYNC_SET_DIM,
         async->layout.dim = dim);
}

void ax__async_set_tree(struct ax_async* async, struct ax_tree* new_tree)
{
    LOCK(async->layout.tree,
         SEND_SYNC(async->layout,
                   async->layout.on_tree_modify,
                   ASYNC_SET_TREE,
                   async->layout.tree = new_tree));
}

void ax__async_set_backend(struct ax_async* async, struct ax_backend* bac)
{
    SEND(async->ui,
         ASYNC_SET_BACKEND,
         async->ui.backend = bac);
}

void ax__async_wait_for_layout(struct ax_async* async)
{
    SEND_SYNC(async->layout,
              async->layout.on_layout,
              ASYNC_WAIT_FOR_LAYOUT,
              {});
}

void ax__async_wait_for_close(struct ax_async* async)
{
    SEND_SYNC(async->ui,
              async->ui.on_close,
              ASYNC_WAIT_FOR_CLOSE,
              {});
}
