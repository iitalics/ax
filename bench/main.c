#include <stdio.h>
#include <string.h>
#include "../src/ax.h"
#include "../src/core.h"
#include "../src/utils.h"
#include "../backend/fortest.h"

#define PRINTF(...) do { printf(__VA_ARGS__); fflush(stdout); } while(0)

static char _pfx[256];
static const char* prefix(long iter, long step)
{
    sprintf(_pfx, "[iter=%ld,step=%ld]", iter + 1, step + 1);
    return _pfx;
}

static void build_wide_tree(struct ax_state* ax)
{
    ax_write_start(ax);
    ax_write_string(ax, "(set-root (container (children");
    for (int i = 0; i < 5000; i++) {
        ax_write_string(ax, "(rect (size 5 6))");
    }
    ax_write_string(ax, ") multi-line (main-justify between)))");
    int r = ax_write_end(ax);
    ASSERT(r == 0, "write failed: %s", ax_get_error(ax));
}

static void repeated_resize(struct ax_state* ax, const char* prefix)
{
    ASSERT(ax->backend != NULL, "backend not initialized");
    for (int i = 0; i < 1000; i++) {
        struct ax_dim size;
        size.w = 100.0 + i * 20.0;
        size.h = 2000.0 - i * 1.8;
        ax_test_backend_sig_resize(ax->backend, size);
        ax_test_backend_sync(ax->backend);
        if (i % 90 == 0) {
            PRINTF("%s %d%%\n", prefix, i / 10);
        }
    }
}

int main(int argc, char** argv)
{
    long iterations = 1;
    if (argc > 1) {
        iterations = strtol(argv[1], NULL, 10);
    }

    for (long i = 0; i < iterations; i++) {
        struct ax_state* ax = ax_new_state();
        PRINTF("%s\n", prefix(i, 0));
        ax_write(ax, "(init)");
        PRINTF("%s\n", prefix(i, 1));
        build_wide_tree(ax);
        PRINTF("%s\n", prefix(i, 2));
        repeated_resize(ax, prefix(i, 2));
        PRINTF("%s\n", prefix(i, 3));
        ax_destroy_state(ax);
    }
    return 0;
}
