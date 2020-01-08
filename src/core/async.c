#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "async.h"
#include "../backend.h"
#include "../core.h"
#include "../tree.h"
#include "../geom.h"
#include "../utils.h"

enum async_msg_type {
    AM_QUIT = 0,
    AM_SET_DIM,
    AM_SET_BACKEND,
    AM_SWAP_TREE,
    AM_SWAP_DRAW_BUF,
    AM_WAIT_FOR_LAYOUT,
    AM_EVT_WAKE_UP,
    AM__MAX,
};

struct async_msg {
    enum async_msg_type ty;
    union {
        struct ax_dim dim;
        struct ax_backend* backend;
    };
};

static void* layout_thd(void*);
static void* ui_thd(void*);
static void* evt_thd(void*);

static void start_mqt(struct ax_async_mqt* t,
                      const char* name,
                      void* (*func)(void*),
                      struct ax_async* async)
{
    t->name = name;
    ax__init_growable(&t->msgs, 4 * sizeof(struct async_msg));
    pthread_mutex_init(&t->msgs_mx, NULL);
    pthread_cond_init(&t->msgs_cv, NULL);
    pthread_create(&t->thd_id, NULL, func, async);
}

static void join_mqt(struct ax_async_mqt* t)
{
    void* ret = NULL;
    pthread_join(t->thd_id, &ret);
    ASSERT(ret == t, "'%s' thread didn't return correctly", t->name);

    pthread_cond_destroy(&t->msgs_cv);
    pthread_mutex_destroy(&t->msgs_mx);
    ax__free_growable(&t->msgs);
}

static inline struct async_msg* mqt_start_send(struct ax_async_mqt* t)
{
    pthread_mutex_lock(&t->msgs_mx);
    return ax__growable_extend(&t->msgs, sizeof(struct async_msg));
}

static inline void mqt_finish_send(struct ax_async_mqt* t)
{
    pthread_cond_signal(&t->msgs_cv);
    pthread_mutex_unlock(&t->msgs_mx);
}

static inline size_t mqt_start_recv(struct ax_async_mqt* t, bool wait)
{
    pthread_mutex_lock(&t->msgs_mx);
    if (wait && ax__is_growable_empty(&t->msgs)) {
        pthread_cond_wait(&t->msgs_cv, &t->msgs_mx);
    }
    size_t n = LEN(&t->msgs, struct async_msg);
    ax__growable_clear(&t->msgs);
    return n;
}

static inline
struct async_msg* mqt_inbox(struct ax_async_mqt* t, size_t i)
{
    return &((struct async_msg*) t->msgs.data)[i];
}

static inline void mqt_finish_recv(struct ax_async_mqt* t)
{
    pthread_mutex_unlock(&t->msgs_mx);
}

static inline void mqt_send_quit(struct ax_async_mqt* t)
{
    struct async_msg* msg = mqt_start_send(t);
    msg->ty = AM_QUIT;
    mqt_finish_send(t);
}

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
    pthread_mutex_init(&async->layout.on_layout_mx, NULL);
    pthread_cond_init(&async->layout.on_layout, NULL);

    // ui
    ax__init_draw_buf(&async->ui.disp_draw_buf);
    ax__init_draw_buf(&async->ui.in_draw_buf);

    // evt
    async->evt.write_fd = evt_write_fd;
    async->evt.pending_close_evt = false;

    start_mqt(&async->layout.t, "layout", layout_thd, async);
    start_mqt(&async->ui.t, "ui", ui_thd, async);
    start_mqt(&async->evt.t, "evt", evt_thd, async);
}

void ax__free_async(struct ax_async* async)
{
    mqt_send_quit(&async->layout.t);
    mqt_send_quit(&async->ui.t);
    mqt_send_quit(&async->evt.t);
    join_mqt(&async->layout.t);
    join_mqt(&async->ui.t);
    join_mqt(&async->evt.t);

    pthread_cond_destroy(&async->layout.on_layout);
    pthread_mutex_destroy(&async->layout.on_layout_mx);
    ax__free_tree(&async->layout.in_tree);
    ax__free_draw_buf(&async->layout.draw_buf);

    ax__free_draw_buf(&async->ui.in_draw_buf);
    ax__free_draw_buf(&async->ui.disp_draw_buf);
}

static void* layout_thd(void* ud)
{
    struct ax_async* async = ud;
    for (bool quit = false; !quit; ) {
        bool swap_tree = false;
        bool needs_layout = false;
        bool notify_about_layout = false;

        size_t n_msgs = mqt_start_recv(&async->layout.t, true);
        for (size_t i = 0; i < n_msgs; i++) {
            struct async_msg* msg = mqt_inbox(&async->layout.t, i);
            switch (msg->ty) {
            case AM_QUIT:
                quit = true;
                break;
            case AM_SET_DIM:
                needs_layout = true;
                async->layout.geom->root_dim = msg->dim;
                break;
            case AM_SWAP_TREE:
                needs_layout = true;
                swap_tree = true;
                break;
            case AM_WAIT_FOR_LAYOUT:
                notify_about_layout = true;
                break;
            default: break;
            }
        }
        if (swap_tree) {
            ax__swap_trees(async->layout.tree, &async->layout.in_tree);
        }
        mqt_finish_recv(&async->layout.t);

        if (needs_layout) {
            ax__layout(async->layout.tree, async->layout.geom);
            ax__redraw(async->layout.tree, &async->layout.draw_buf);

            struct async_msg* msg = mqt_start_send(&async->ui.t);
            msg->ty = AM_SWAP_DRAW_BUF;
            ax__swap_draw_bufs(&async->ui.in_draw_buf, &async->layout.draw_buf);
            mqt_finish_send(&async->ui.t);
        }

        if (notify_about_layout) {
            pthread_mutex_lock(&async->layout.on_layout_mx);
            pthread_cond_broadcast(&async->layout.on_layout);
            pthread_mutex_unlock(&async->layout.on_layout_mx);
        }
    }
    return &async->layout.t;
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
        bool swap_draw_buf = false;

        size_t n_msgs = mqt_start_recv(&async->ui.t, bac == NULL);
        for (size_t i = 0; i < n_msgs; i++) {
            struct async_msg* msg = mqt_inbox(&async->ui.t, i);
            switch (msg->ty) {
            case AM_QUIT:
                quit = true;
                break;
            case AM_SET_BACKEND:
                bac = msg->backend;
                break;
            case AM_SWAP_DRAW_BUF:
                swap_draw_buf = true;
                break;
            default: break;
            }
        }
        if (swap_draw_buf) {
            ax__swap_draw_bufs(&async->ui.disp_draw_buf,
                               &async->ui.in_draw_buf);
        }
        mqt_finish_recv(&async->ui.t);

        if (bac != NULL) {
            ui_thd_step(async, bac);
        }
    }
    return &async->ui.t;
}

static void* evt_thd(void* ud)
{
    struct ax_async* async = ud;
    bool quit = false;
    while (!quit) {
        size_t n_msgs = mqt_start_recv(&async->evt.t, true);
        for (size_t i = 0; i < n_msgs; i++) {
            struct async_msg* msg = mqt_inbox(&async->evt.t, i);
            switch (msg->ty) {
            case AM_QUIT:
                quit = true;
                break;
            default: break;
            }
        }
        bool close_evt = async->evt.pending_close_evt;
        async->evt.pending_close_evt = false;
        mqt_finish_recv(&async->evt.t);

        if (close_evt) {
            write(async->evt.write_fd, "C", 1);
        }
    }
    return &async->evt.t;
}

void ax__async_set_dim(struct ax_async* async, struct ax_dim dim)
{
    struct async_msg* msg = mqt_start_send(&async->layout.t);
    msg->ty = AM_SET_DIM;
    msg->dim = dim;
    mqt_finish_send(&async->layout.t);
}

void ax__async_set_tree(struct ax_async* async, struct ax_tree* new_tree)
{
    struct async_msg* msg = mqt_start_send(&async->layout.t);
    msg->ty = AM_SWAP_TREE;
    ax__swap_trees(&async->layout.in_tree, new_tree);
    mqt_finish_send(&async->layout.t);
    ax__tree_clear(new_tree);
}

void ax__async_set_backend(struct ax_async* async, struct ax_backend* bac)
{
    struct async_msg* msg = mqt_start_send(&async->ui.t);
    msg->ty = AM_SET_BACKEND;
    msg->backend = bac;
    mqt_finish_send(&async->ui.t);
}

void ax__async_wait_for_layout(struct ax_async* async)
{
    struct async_msg* msg = mqt_start_send(&async->layout.t);
    msg->ty = AM_WAIT_FOR_LAYOUT;
    pthread_mutex_lock(&async->layout.on_layout_mx);
    mqt_finish_send(&async->layout.t);
    pthread_cond_wait(&async->layout.on_layout, &async->layout.on_layout_mx);
    pthread_mutex_unlock(&async->layout.on_layout_mx);
}

void ax__async_push_close_evt(struct ax_async* async)
{
    struct async_msg* msg = mqt_start_send(&async->evt.t);
    msg->ty = AM_EVT_WAKE_UP;
    async->evt.pending_close_evt = true;
    mqt_finish_send(&async->evt.t);
}
