#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "async.h"
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

#define RECV(sys, _msg, _body)                      \
    LOCK(sys.msg, {                                 \
            if (sys.msg == 0) {                     \
                pthread_cond_wait(&sys.new_msg_cv,  \
                                  &sys.msg_mx);     \
            }                                       \
            _msg = sys.msg;                         \
            sys.msg = 0;                            \
            _body;                                  \
        });

static void* layout_thd(void*);

void ax__init_async(struct ax_state* s, struct ax_async* async)
{
    async->geom = s->geom;
    async->tree = s->tree;

    pthread_mutex_init(&async->layout.msg_mx, NULL);
    pthread_mutex_init(&async->layout.tree_mx, NULL);
    pthread_mutex_init(&async->layout.tree_modify_mx, NULL);
    pthread_mutex_init(&async->layout.wait_for_mx, NULL);
    pthread_cond_init(&async->layout.new_msg_cv, NULL);
    pthread_cond_init(&async->layout.tree_modify, NULL);
    pthread_cond_init(&async->layout.wait_for, NULL);

    pthread_create(&async->layout.thd, NULL, layout_thd, (void*) async);
}

void ax__free_async(struct ax_async* async)
{
    SEND(async->layout, ASYNC_QUIT, {});

    void* ret;
    int r = pthread_join(async->layout.thd, &ret);
    ASSERT(r == 0, "%s", strerror(errno));
    ASSERT(ret == &async->layout, "thread didn't exit properly");

    pthread_cond_destroy(&async->layout.wait_for);
    pthread_cond_destroy(&async->layout.tree_modify);
    pthread_cond_destroy(&async->layout.new_msg_cv);
    pthread_mutex_destroy(&async->layout.wait_for_mx);
    pthread_mutex_destroy(&async->layout.tree_modify_mx);
    pthread_mutex_destroy(&async->layout.tree_mx);
    pthread_mutex_destroy(&async->layout.msg_mx);
}

static void* layout_thd(void* ud)
{
    struct ax_async* async = ud;

    for (bool quit = false; !quit; ) {
        bool needs_layout = false;
        bool notify_about_layout = false;
        int msg;
        RECV(async->layout, msg, {
                if (msg & ASYNC_QUIT) {
                    quit = true;
                }
                if (msg & ASYNC_SET_DIM) {
                    needs_layout = true;
                    async->geom->root_dim = async->layout.dim;
                }
                if (msg & ASYNC_SET_TREE) {
                    needs_layout = true;
                    ax__tree_drain_from(async->tree, async->layout.tree);
                    NOTIFY(async->layout.tree_modify);
                }
                if (msg & ASYNC_WAIT_FOR_LAYOUT) {
                    notify_about_layout = true;
                }
            });

        if (needs_layout) {
            ax__layout(async->tree, async->geom);
            needs_layout = false;
        }

        if (notify_about_layout) {
            NOTIFY(async->layout.wait_for);
        }
    }

    return &async->layout;
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
                   async->layout.tree_modify,
                   ASYNC_SET_TREE,
                   async->layout.tree = new_tree));
}

void ax__async_wait_for_layout(struct ax_async* async)
{
    SEND_SYNC(async->layout,
              async->layout.wait_for,
              ASYNC_WAIT_FOR_LAYOUT,
              {});
}
