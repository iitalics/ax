#include "helpers.h"
#include "../src/ax.h"
#include "../src/core.h"
#include "../src/tree.h"

#define N(_id)  ax__node_by_id(s->tree, _id)

TEST(empty_root_node)
{
    struct ax_state* s = ax_new_state();
    CHECK_SZEQ(s->tree->count, (size_t) 1);
    CHECK_IEQ(N(0)->ty, AX_NODE_CONTAINER);
    ax_destroy_state(s);
}

#define TWO_RECTS                               \
    "(rect (fill \"ff0000\") (size 60 60))"     \
    "(rect (fill \"0000ff\") (size 60 60))"

TEST(build_tree)
{
    struct ax_state* s = ax_new_state();
    ax_write(s,
             "(init (window-size 200 200))"
             "(set-root"
             " (container (children " TWO_RECTS ")))");
    CHECK_SZEQ(s->tree->count, (size_t) 3);
    CHECK_IEQ(N(0)->ty, AX_NODE_CONTAINER);
    CHECK_IEQ(N(1)->ty, AX_NODE_RECTANGLE);
    CHECK_IEQ(N(2)->ty, AX_NODE_RECTANGLE);
    CHECK_IEQ_HEX(N(1)->r.fill, 0xff0000);
    CHECK_IEQ_HEX(N(2)->r.fill, 0x0000ff);
    CHECK_DIMEQ(N(1)->r.size, AX_DIM(60, 60));
    CHECK_DIMEQ(N(2)->r.size, AX_DIM(60, 60));
    ax_destroy_state(s);
}

/* Justification tests with two 60x60 rectangles, in a 200x200 window */

#define JUSTIFY_TEST_2R(_mj, _xj, _x0, _y0, _x1, _y1) do {          \
        struct ax_state* s = ax_new_state();                        \
        ax_write(s,                                                 \
                 "(init (window-size 200 200))"                                \
                 "(set-root (container (children " TWO_RECTS ")"    \
                 "                     (main-justify " _mj ")"      \
                 "                     (cross-justify " _xj ")))"); \
        CHECK_POSEQ(N(1)->coord, AX_POS(_x0, _y0));                 \
        CHECK_POSEQ(N(2)->coord, AX_POS(_x1, _y1));                 \
        ax_destroy_state(s);                                        \
    } while(0)

TEST(main_justify_start_2r)
{ JUSTIFY_TEST_2R("start", "start", 0, 0, 60, 0); }

TEST(main_justify_end_2r)
{ JUSTIFY_TEST_2R("end", "start", 80, 0, 140, 0); }

TEST(main_justify_center_2r)
{ JUSTIFY_TEST_2R("center", "start", 40, 0, 100, 0); }

TEST(main_justify_between_2r)
{ JUSTIFY_TEST_2R("between", "start", 0, 0, 140, 0); }

TEST(main_justify_even_2r)
{ JUSTIFY_TEST_2R("evenly", "start", 26.666, 0, 113.333, 0); }

TEST(main_justify_around_2r)
{ JUSTIFY_TEST_2R("around", "start", 20, 0, 120, 0); }

#undef JUSTIFY_TEST_2R


/* Fitting a single text node into a window */

#define TEXT_TEST(_aw, _ah, _fsz, _str, _expw, _exph)   \
    struct ax_state* s = ax_new_state();                \
    ax_write(s,                                         \
             "(init (window-size " # _aw " " # _ah "))" \
             "(set-root"                                \
             " (text \"" _str "\""                      \
             "       (font \"size:" # _fsz "\")))");    \
    CHECK_SZEQ(s->tree->count, (size_t) 1);             \
    CHECK_IEQ(N(0)->ty, AX_NODE_TEXT);                  \
    CHECK_FLEQ(0.001, N(0)->hypoth.w, (float) (_expw)); \
    CHECK_FLEQ(0.001, N(0)->hypoth.h, (float) (_exph)); \
    ax_destroy_state(s)

TEST(text_geom_2w_1l)
{ TEXT_TEST(200, 200, 10, "Hello, world", 120, 10); }

TEST(text_geom_2w_2l)
{ TEXT_TEST(100, 100, 10, "Hello, world", 60, 20); }

TEST(text_geom_3w_2l)
{ TEXT_TEST(100, 100, 10, "Hello, za world", 90, 20); }

#undef TEXT_TEST


TEST(spill_3r)
{
    struct ax_state* s = ax_new_state();
    ax_write(s,
            "(init (window-size 200 200))"
            "(set-root"
            " (container"
            "  (children (rect (fill \"ff0000\") (size 80 80))"
            "            (rect (fill \"00ff00\") (size 80 80))"
            "            (rect (fill \"0000ff\") (size 80 80)))"
            "  multi-line))");
    CHECK_SZEQ(s->tree->count, (size_t) 4);
    CHECK_SZEQ(N(0)->c.n_lines, (size_t) 2);
    CHECK_SZEQ(N(0)->c.line_count[0], (size_t) 2);
    CHECK_SZEQ(N(0)->c.line_count[1], (size_t) 1);
    CHECK_POSEQ(N(1)->coord, AX_POS(0.0, 0.0));
    CHECK_POSEQ(N(2)->coord, AX_POS(80.0, 0.0));
    CHECK_POSEQ(N(3)->coord, AX_POS(0.0, 80.0));
}

TEST(shrink_3r)
{
    struct ax_state* s = ax_new_state();
    ax_write(s,
            "(init (window-size 200 200))"
            "(set-root"
            " (container"
            "  (children (rect (fill \"ff0000\") (size 80 80))"
            "            (rect (fill \"00ff00\") (size 80 80))"
            "            (rect (fill \"0000ff\") (size 80 80)))"
            "  single-line))");
    CHECK_SZEQ(s->tree->count, (size_t) 4);
    CHECK_SZEQ(N(0)->c.n_lines, (size_t) 1);
    CHECK_SZEQ(N(0)->c.line_count[0], (size_t) 3);
    CHECK_POSEQ(N(1)->coord, AX_POS(0.0, 0.0));
    CHECK_POSEQ(N(2)->coord, AX_POS(66.66, 0.0));
    CHECK_POSEQ(N(3)->coord, AX_POS(133.33, 0.0));
    CHECK_DIMEQ(N(1)->target, AX_DIM(66.66, 80.0));
    CHECK_DIMEQ(N(2)->target, AX_DIM(66.66, 80.0));
    CHECK_DIMEQ(N(3)->target, AX_DIM(66.66, 80.0));
    ax_destroy_state(s);
}

TEST(shrink_3r_asym)
{
    struct ax_state* s = ax_new_state();
    ax_write(s,
             "(init (window-size 200 200))"
             "(set-root"
             " (container"
             "  (children (rect (fill \"ff0000\") (size 80 80) (shrink 0))"
             "            (rect (fill \"00ff00\") (size 80 80))"
             "            (rect (fill \"0000ff\") (size 80 80)))"
             "  single-line))");
    CHECK_SZEQ(s->tree->count, (size_t) 4);
    CHECK_SZEQ(N(0)->c.n_lines, (size_t) 1);
    CHECK_SZEQ(N(0)->c.line_count[0], (size_t) 3);
    CHECK_POSEQ(N(1)->coord, AX_POS(0.0, 0.0));
    CHECK_POSEQ(N(2)->coord, AX_POS(80.0, 0.0));
    CHECK_POSEQ(N(3)->coord, AX_POS(140, 0.0));
    CHECK_DIMEQ(N(1)->target, AX_DIM(80.0, 80.0)); // shrink_factor=0
    CHECK_DIMEQ(N(2)->target, AX_DIM(60.0, 80.0));
    CHECK_DIMEQ(N(3)->target, AX_DIM(60.0, 80.0));
    ax_destroy_state(s);
}
