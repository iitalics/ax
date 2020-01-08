#pragma once
#include <pthread.h>
#include "../backend.h"
#include "../draw.h"
#include "../tree.h"
#include "../core/growable.h"

struct ax_state;
struct ax_geom;
struct ax_tree;

struct ax_async_mqt {
    const char* name;
    pthread_t thd_id;
    struct growable msgs;
    pthread_mutex_t msgs_mx;
    pthread_cond_t msgs_cv;
};

struct ax_async {
    struct {
        struct ax_geom* geom;
        struct ax_tree* tree;

        struct ax_async_mqt t;
        struct ax_draw_buf draw_buf;
        struct ax_tree in_tree;
        pthread_cond_t on_layout;
        pthread_mutex_t on_layout_mx;
    } layout;

    struct {
        struct ax_async_mqt t;
        struct ax_draw_buf disp_draw_buf;
        struct ax_draw_buf in_draw_buf;
    } ui;

    struct {
        int write_fd;

        struct ax_async_mqt t;
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
