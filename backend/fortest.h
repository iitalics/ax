#pragma once
#include <pthread.h>
#include "../src/backend.h"
#include "../src/core/region.h"

struct ax_backend {
    struct ax_draw* ds;
    size_t ds_len;

    pthread_mutex_t sig_mx;
    struct {
        bool close;
    } sig;

    pthread_cond_t sync;
    pthread_mutex_t sync_mx;

    struct region font_rgn;
};

void ax_test_backend_sync_until(struct ax_backend* bac, size_t desired_len);
void ax_test_backend_sig_close(struct ax_backend* bac);
