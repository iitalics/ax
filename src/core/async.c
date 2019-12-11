#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "async.h"
#include "../core.h"
#include "../tree.h"
#include "../geom.h"
#include "../utils.h"

static int layout_thd_exit_ok;
static void* layout_thd_worker(void*);

#define LAYOUT_LOCK(async, _body) do {              \
        pthread_mutex_lock(&async->layout_mx);      \
        _body;                                      \
        pthread_mutex_unlock(&async->layout_mx);    \
    } while(0)

#define LAYOUT_SIGNAL(async, _sig, _prep)               \
    LAYOUT_LOCK(async, {                                \
            async->layout_sig = (_sig);                 \
            _prep;                                      \
            pthread_cond_signal(&async->layout_cv);     \
            pthread_cond_wait(&async->layout_ack_cv,    \
                              &async->layout_mx);       \
        })

void ax__init_async(struct ax_state* s, struct ax_async* async)
{
    async->geom = s->geom;
    async->tree = s->tree;

    pthread_mutex_init(&async->layout_mx, NULL);
    pthread_cond_init(&async->layout_cv, NULL);
    pthread_cond_init(&async->layout_ack_cv, NULL);
    pthread_create(&async->layout_thd, NULL, layout_thd_worker, (void*) async);
    LAYOUT_LOCK(async, pthread_cond_wait(&async->layout_ack_cv, &async->layout_mx));
}

void ax__free_async(struct ax_async* async)
{
    LAYOUT_SIGNAL(async, AX_LS_QUIT, {});

    void* ret;
    int r = pthread_join(async->layout_thd, &ret);
    ASSERT(r == 0, "%s", strerror(errno));
    ASSERT(ret == &layout_thd_exit_ok, "thread didn't exit properly");

    pthread_cond_destroy(&async->layout_cv);
    pthread_mutex_destroy(&async->layout_mx);
}

static void layout_thd_recv(struct ax_async* async,
                            bool* quit,
                            bool* invalidated)
{
    switch (async->layout_sig) {
    case AX_LS_QUIT:
        *quit = true;
        break;

    case AX_LS_SET_TREE:
        ax__tree_drain_from(async->tree, async->new_tree);
        *invalidated = true;
        break;

    case AX_LS_SET_DIM:
        async->geom->root_dim = async->dim;
        *invalidated = true;
        break;

    default: NO_SUCH_TAG("ax_layout_sig");
    }
}

static void* layout_thd_worker(void* ud)
{
    struct ax_async* async = ud;
    LAYOUT_LOCK(async, {
            pthread_cond_signal(&async->layout_ack_cv);
            for (bool quit = false; !quit; ) {
                pthread_cond_wait(&async->layout_cv, &async->layout_mx);
                bool inval = false;
                layout_thd_recv(async, &quit, &inval);
                pthread_cond_signal(&async->layout_ack_cv);

                if (inval && !quit) {
                    ax__layout(async->tree, async->geom);
                }
            }
        });
    return &layout_thd_exit_ok;
}

void ax__sync_set_tree(struct ax_async* async, struct ax_tree* new_tree)
{
    LAYOUT_SIGNAL(async, AX_LS_SET_TREE, async->new_tree = new_tree);
}

void ax__sync_set_dim(struct ax_async* async, struct ax_dim dim)
{
    LAYOUT_SIGNAL(async, AX_LS_SET_DIM, async->dim = dim);
}
