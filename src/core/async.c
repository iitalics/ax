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
static void* evt_thd(void*);

void ax__init_async(struct ax_async* async,
                    struct ax_geom* geom_subsys,
                    struct ax_tree* tree_subsys,
                    int evt_write_fd)
{
    // layout
    async->layout.geom = geom_subsys;
    async->layout.tree = tree_subsys;
    ax__init_draw_buf(&async->layout.draw_buf);
    ax__init_tree(&async->layout.in_tree);
    async->layout.msg = 0;
    pthread_mutex_init(&async->layout.msg_mx, NULL);
    pthread_mutex_init(&async->layout.on_layout_mx, NULL);
    pthread_cond_init(&async->layout.new_msg_cv, NULL);
    pthread_cond_init(&async->layout.on_layout, NULL);
    pthread_create(&async->layout.thd, NULL, layout_thd, (void*) async);

    // ui
    ax__init_draw_buf(&async->ui.disp_draw_buf);
    ax__init_draw_buf(&async->ui.in_draw_buf);
    async->ui.msg = 0;
    pthread_mutex_init(&async->ui.msg_mx, NULL);
    pthread_mutex_init(&async->ui.on_close_mx, NULL);
    pthread_cond_init(&async->ui.new_msg_cv, NULL);
    pthread_cond_init(&async->ui.on_close, NULL);
    pthread_create(&async->ui.thd, NULL, ui_thd, (void*) async);

    // evt
    async->evt.write_fd = evt_write_fd;
    async->evt.msg = 0;
    async->evt.pending_close_evt = false;
    pthread_mutex_init(&async->evt.msg_mx, NULL);
    pthread_cond_init(&async->evt.new_msg_cv, NULL);
    pthread_create(&async->evt.thd, NULL, evt_thd, (void*) async);
}

void ax__free_async(struct ax_async* async)
{
    SEND(async->layout, ASYNC_QUIT, {});
    SEND(async->ui, ASYNC_QUIT, {});
    SEND(async->evt, ASYNC_QUIT, {});

    JOIN(async->layout);
    JOIN(async->ui);
    JOIN(async->evt);

    pthread_cond_destroy(&async->layout.on_layout);
    pthread_cond_destroy(&async->layout.new_msg_cv);
    pthread_mutex_destroy(&async->layout.on_layout_mx);
    pthread_mutex_destroy(&async->layout.msg_mx);
    ax__free_tree(&async->layout.in_tree);
    ax__free_draw_buf(&async->layout.draw_buf);

    pthread_cond_destroy(&async->ui.on_close);
    pthread_cond_destroy(&async->ui.new_msg_cv);
    pthread_mutex_destroy(&async->ui.on_close_mx);
    pthread_mutex_destroy(&async->ui.msg_mx);
    ax__free_draw_buf(&async->ui.in_draw_buf);
    ax__free_draw_buf(&async->ui.disp_draw_buf);

    pthread_cond_destroy(&async->evt.new_msg_cv);
    pthread_mutex_destroy(&async->evt.msg_mx);
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
        async->layout.geom->root_dim = async->layout.in_dim;
    }
    if (msg & ASYNC_SWAP_TREES) {
        *out_needs_layout = true;
        ax__swap_trees(async->layout.tree, &async->layout.in_tree);
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
            ax__layout(async->layout.tree, async->layout.geom);
            ax__redraw(async->layout.tree, &async->layout.draw_buf);
            SEND(async->ui, ASYNC_SWAP_BUFFERS,
                 ax__swap_draw_bufs(&async->ui.in_draw_buf,
                                    &async->layout.draw_buf));
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
        *out_bac = async->ui.in_backend;
    }
    if (msg & ASYNC_SWAP_BUFFERS) {
        ax__swap_draw_bufs(&async->ui.disp_draw_buf, &async->ui.in_draw_buf);
    }
}

static void ui_thd_step(struct ax_async* async, struct ax_backend* bac)
{
    struct ax_backend_evt e;
    while (ax__poll_event(bac, &e)) {
        switch (e.ty) {

        case AX_BEVT_CLOSE:
            ax__async_push_close_evt(async);
            break;

        case AX_BEVT_RESIZE:
            ax__async_set_dim(async, e.resize_dim);
            break;

        default: NO_SUCH_TAG("ax_backend_evt_type");
        }
    }

    ax__render(bac,
               ax__draw_buf_data(&async->ui.disp_draw_buf),
               ax__draw_buf_count(&async->ui.disp_draw_buf));
    ax__wait_for_frame(bac);
}

static void* ui_thd(void* ud)
{
    struct ax_async* async = ud;
    struct ax_backend* bac = NULL;
    bool quit = false;
    while (!quit) {
        if (bac != NULL) {
            ui_thd_step(async, bac);
        }

        int msg;
        RECV(async->ui, msg, bac == NULL,
             ui_thd_handle(async, msg, &quit, &bac));
    }
    return &async->ui;
}

static void* evt_thd(void* ud)
{
    struct ax_async* async = ud;
    bool quit = false;
    while (!quit) {
        bool close_evt;
        int msg;
        RECV(async->evt, msg, true, {
                if (msg & ASYNC_QUIT) {
                    quit = true;
                }
                close_evt = async->evt.pending_close_evt;
                async->evt.pending_close_evt = false;
            });

        if (close_evt) {
            write(async->evt.write_fd, "C", 1);
        }
    }
    return &async->evt;
}

void ax__async_set_dim(struct ax_async* async, struct ax_dim dim)
{
    SEND(async->layout,
         ASYNC_SET_DIM,
         async->layout.in_dim = dim);
}

void ax__async_set_tree(struct ax_async* async, struct ax_tree* new_tree)
{
    SEND(async->layout,
         ASYNC_SWAP_TREES,
         ax__swap_trees(&async->layout.in_tree, new_tree));
    ax__tree_clear(new_tree);
}

void ax__async_set_backend(struct ax_async* async, struct ax_backend* bac)
{
    SEND(async->ui,
         ASYNC_SET_BACKEND,
         async->ui.in_backend = bac);
}

void ax__async_wait_for_layout(struct ax_async* async)
{
    SEND_SYNC(async->layout,
              async->layout.on_layout,
              ASYNC_WAIT_FOR_LAYOUT,
              {});
}

void ax__async_push_close_evt(struct ax_async* async)
{
    SEND(async->evt,
         ASYNC_WAKE_UP,
         async->evt.pending_close_evt = true);
}
