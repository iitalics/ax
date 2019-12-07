#include "helpers.h"
#include "../src/ax.h"

TEST(die)
{
    struct ax_state* s = ax_new_state();
    CHECK_NULL(ax_get_error(s));
    ax_write(s, "(die \"oof ouch\")");
    CHECK_STREQ(ax_get_error(s), "oof ouch");
    ax_destroy_state(s);
}
