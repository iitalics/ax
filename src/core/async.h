#pragma once
#include <pthread.h>
#include "../base.h"
#include "../draw.h"

struct ax_state;
struct ax_geom;
struct ax_tree;
struct ax_backend;

enum {
    ASYNC_QUIT            = 1 << 1,
    ASYNC_SET_DIM         = 1 << 2,
    ASYNC_SET_TREE        = 1 << 3,
    ASYNC_WAIT_FOR_LAYOUT = 1 << 4,
    ASYNC_SET_BACKEND     = 1 << 5,
    ASYNC_FLIP_BUFFERS    = 1 << 6,
};

struct ax_async {
    struct ax_geom* geom;
    struct ax_tree* tree;

#define MESSAGE_QUEUE_VARS()                    \
    int volatile msg;                           \
    pthread_mutex_t msg_mx;                     \
    pthread_cond_t new_msg_cv;

    struct {
        pthread_t thd;
        struct ax_draw_buf db;
        MESSAGE_QUEUE_VARS();

        struct ax_dim volatile dim;

        struct ax_tree* volatile tree;
        pthread_mutex_t tree_mx;
        pthread_cond_t tree_modify;
        pthread_mutex_t tree_modify_mx;

        pthread_cond_t wait_for;
        pthread_mutex_t wait_for_mx;
    } layout;

    struct {
        pthread_t thd;
        MESSAGE_QUEUE_VARS();

        struct ax_backend* backend;

        struct ax_draw_buf display_db;
        struct ax_draw_buf scratch_db;
        pthread_mutex_t scratch_db_mx;
    } ui;
};

void ax__init_async(struct ax_state* s, struct ax_async* async);
void ax__free_async(struct ax_async* async);

void ax__async_set_dim(struct ax_async* async, struct ax_dim dim);
void ax__async_set_tree(struct ax_async* async, struct ax_tree* new_tree);
void ax__async_wait_for_layout(struct ax_async* async);

void ax__async_set_backend(struct ax_async* async, struct ax_backend* bac);
