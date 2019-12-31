#pragma once
#include <pthread.h>
#include "../backend.h"
#include "../draw.h"

struct ax_state;
struct ax_geom;
struct ax_tree;

enum {
    ASYNC_QUIT            = 1 << 1,
    ASYNC_SET_DIM         = 1 << 2,
    ASYNC_SET_TREE        = 1 << 3,
    ASYNC_SET_BACKEND     = 1 << 4,
    ASYNC_FLIP_BUFFERS    = 1 << 5,
    ASYNC_WAIT_FOR_LAYOUT = 1 << 6,
    ASYNC_PUSH_BEVT       = 1 << 7,
    ASYNC_POP_BEVT        = 1 << 8,
};

#define MESSAGE_QUEUE_VARS()                    \
    int volatile msg;                           \
    pthread_mutex_t msg_mx;                     \
    pthread_cond_t new_msg_cv;

struct ax_async {
    struct {
        pthread_t thd;
        struct ax_geom* geom;
        struct ax_tree* tree;
        struct ax_draw_buf db;
        MESSAGE_QUEUE_VARS();

        struct ax_dim volatile dim;

        struct ax_tree* volatile in_tree;
        pthread_mutex_t in_tree_mx;
        pthread_cond_t in_tree_drained;
        pthread_mutex_t in_tree_drained_mx;

        pthread_cond_t on_layout;
        pthread_mutex_t on_layout_mx;
    } layout;

    struct {
        pthread_t thd;
        MESSAGE_QUEUE_VARS();

        struct ax_backend* backend;

        struct ax_draw_buf display_db;
        struct ax_draw_buf scratch_db;
        pthread_mutex_t scratch_db_mx;

        pthread_cond_t on_close;
        pthread_mutex_t on_close_mx;
    } ui;

    struct {
        pthread_t thd;
        int write_fd;
        MESSAGE_QUEUE_VARS();

        int volatile bevt_ty_mask;
        struct ax_dim resize_dim;
    } evt;
};

void ax__init_async(struct ax_async* async,
                    struct ax_geom* geom_subsys,
                    struct ax_tree* tree_subsys,
                    int evt_write_fd);
void ax__free_async(struct ax_async* async);

void ax__async_set_dim(struct ax_async* async, struct ax_dim dim);
void ax__async_set_tree(struct ax_async* async, struct ax_tree* new_tree);
void ax__async_set_backend(struct ax_async* async, struct ax_backend* bac);

void ax__async_wait_for_layout(struct ax_async* async);

void ax__async_push_bevt(struct ax_async* async, struct ax_backend_evt e);
bool ax__async_pop_bevt(struct ax_async* async, struct ax_backend_evt* out_e);
