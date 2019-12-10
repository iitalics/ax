#pragma once
#include "../src/backend.h"

struct ax_backend {
    struct ax_draw* ds;
    size_t ds_len;
};
