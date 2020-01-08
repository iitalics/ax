#pragma once
#include <pthread.h>
#include "../backend.h"
#include "../draw.h"
#include "../tree.h"

struct ax_state;
struct ax_geom;
struct ax_tree;

enum {
    // (all subsystems)
    ASYNC_QUIT            = 1 << 1,
    // layout
    ASYNC_SET_DIM         = 1 << 2,
    ASYNC_SWAP_TREES      = 1 << 3,
    ASYNC_WAIT_FOR_LAYOUT = 1 << 6,
    // ui
    ASYNC_SET_BACKEND     = 1 << 4,
    ASYNC_SWAP_BUFFERS    = 1 << 5,
    // evt
    ASYNC_WAKE_UP         = 1 << 7,
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
        struct ax_draw_buf draw_buf;
        MESSAGE_QUEUE_VARS();

        struct ax_dim volatile in_dim;
        struct ax_tree in_tree;

        pthread_cond_t on_layout;
        pthread_mutex_t on_layout_mx;
    } layout;

    struct {
        pthread_t thd;
        struct ax_draw_buf disp_draw_buf;
        MESSAGE_QUEUE_VARS();

        struct ax_backend* in_backend;
        struct ax_draw_buf in_draw_buf;

        pthread_cond_t on_close;
        pthread_mutex_t on_close_mx;
    } ui;

    struct {
        pthread_t thd;
        int write_fd;
        MESSAGE_QUEUE_VARS();

        bool pending_close_evt;
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

void ax__async_push_close_evt(struct ax_async* async);
