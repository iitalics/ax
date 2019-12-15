#pragma once
#include <pthread.h>
#include "../src/backend.h"

struct ax_backend {
    struct ax_draw* ds;
    size_t ds_len;

    pthread_cond_t sync;
    pthread_mutex_t sync_mx;
};

void ax_test_backend_sync_until(struct ax_backend* bac, size_t desired_len);
