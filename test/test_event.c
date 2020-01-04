#include "helpers.h"
#include "../src/ax.h"
#include "../src/core.h"
#include "../backend/fortest.h"

TEST(evt_poll_empty)
{
    struct ax_state* ax = ax_new_state();
    ax_write(ax, "(init)");
    CHECK_FALSE(ax_poll_event(ax));
    ax_destroy_state(ax);
}

TEST(evt_close)
{
    struct ax_state* ax = ax_new_state();
    ax_write(ax, "(init)");
    for (int i = 0; i < 5; i++) {
        CHECK_FALSE(ax_poll_event(ax));
        ax_test_backend_sig_close(ax->backend);
        ax_read_close_event(ax);
    }
    ax_destroy_state(ax);
}
