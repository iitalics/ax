#pragma once
#include <pthread.h>
#include "../base.h"

struct ax_state;
struct ax_geom;
struct ax_tree;

enum ax_layout_sig {
    AX_LS_QUIT = 0,
    AX_LS_SET_TREE,
    AX_LS_SET_DIM,
    AX_LS__MAX,
};

struct ax_async {
    struct ax_geom* geom;
    struct ax_tree* tree;

    pthread_t layout_thd;
    pthread_mutex_t layout_mx;
    pthread_cond_t layout_cv;
    pthread_cond_t layout_ack_cv;
    enum ax_layout_sig layout_sig;
    union {
        struct ax_tree* new_tree;
        struct ax_dim dim;
    };
};

void ax__init_async(struct ax_state* s, struct ax_async* async);
void ax__free_async(struct ax_async* async);

void ax__sync_set_tree(struct ax_async* async, struct ax_tree* new_tree);
void ax__sync_set_dim(struct ax_async* async, struct ax_dim dim);
