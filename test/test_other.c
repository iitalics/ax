#include "helpers.h"
#include "../src/ax.h"
#include "../src/core.h"
#include "../src/tree.h"

TEST(die)
{
    struct ax_state* s = ax_new_state();
    CHECK_NULL(ax_get_error(s));
    int r = ax_write(s, "(die \"oof ouch\")");
    CHECK_IEQ(r, 1);
    CHECK_STREQ(ax_get_error(s), "oof ouch");
    ax_destroy_state(s);
}

TEST(die_set_root_no_init)
{
    struct ax_state* s = ax_new_state();
    int r = ax_write(s, "(set-root (rect))");
    CHECK_IEQ(r, 1);
    ax_destroy_state(s);
}

/* TEST(die_event_loop_no_init) */
/* { */
/*     struct ax_state* s = ax_new_state(); */
/*     int r = ax_event_loop(s); */
/*     CHECK_IEQ(r, 1); */
/*     ax_destroy_state(s); */
/* } */

TEST(die_two_init)
{
    struct ax_state* s = ax_new_state();
    int r = ax_write(s, "(init) (init)");
    CHECK_IEQ(r, 1);
    ax_destroy_state(s);
}

TEST(die_font_bad_spec_error)
{
    struct ax_state* s = ax_new_state();
    CHECK_SZEQ(ax__tree_count(s->tree), (size_t) 0);
    int r = ax_write(s,
                     "(init)"
                     "(set-root"
                     " (container"
                     "  (children (rect) (rect) (rect) (rect)"
                     "            (text \"hello\" (font \"bad\")))))");
    CHECK_IEQ(r, 1);
    CHECK_STREQ(ax_get_error(s), "invalid fake font");
    CHECK_SZEQ(ax__tree_count(s->tree), (size_t) 0); // tree wasn't set
    ax_destroy_state(s);
}

TEST(die_recover)
{
    struct ax_state* s = ax_new_state();
    int r = ax_write(s, "(init) (init)");
    CHECK_IEQ(r, 1);
    CHECK_STREQ(ax_get_error(s), "backend already initialized");
    r = ax_write(s, "(set-root (container (children (rect) (rect))))");
    CHECK_IEQ(r, 0);
    CHECK_SZEQ(ax__tree_count(s->tree), (size_t) 3);
    ax_destroy_state(s);
}

TEST(die_parse_error_fails_early)
{
    struct ax_state* s = ax_new_state();
    int r = ax_write(s, "$ (init)");
    CHECK_IEQ(r, 1);
    CHECK_STREQ(ax_get_error(s), "syntax error: invalid character `$'");
    CHECK_FALSE(ax__is_backend_initialized(s));
    ax_destroy_state(s);
}

TEST(die_syntax_error_fails_early)
{
    struct ax_state* s = ax_new_state();
    int r = ax_write(s, "(foo) (init)");
    CHECK_IEQ(r, 1);
    // TODO: meaningful error message
    CHECK_STREQ(ax_get_error(s), "syntax error: expected ???");
    CHECK_FALSE(ax__is_backend_initialized(s));
    ax_destroy_state(s);
}

TEST(build_big_tree)
{
    struct ax_state* s = ax_new_state();
    ax_write_start(s);
    ax_write_string(s, "(init (window-size 200 200))");
    ax_write_string(s, "(set-root (container (children");
    for (int i = 0; i < 1000; i++) {
        ax_write_string(s, "(rect (size 20 20))");
    }
    ax_write_string(s, ")))");
    CHECK_IEQ(ax_write_end(s), 0);
    CHECK_SZEQ(ax__tree_count(s->tree), (size_t) 1001);
    ax_destroy_state(s);
}
