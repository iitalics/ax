#include "helpers.h"
#include "../src/ax.h"
#include "../src/state.h"
#include "desc_helpers.h"

#define N(_id)  ax_node_by_id(&s->tree, _id)


TEST(empty_root_node)
{
    struct ax_state* s = ax_new_state();
    CHECK_SZEQ(s->tree.count, (size_t) 1);
    CHECK_IEQ(N(0)->ty, AX_NODE_CONTAINER);
    ax_destroy_state(s);
}


TEST(build_tree)
{
    struct ax_state* s = ax_new_state();
    const struct ax_desc root = CONT(
        AX_JUSTIFY_START, 0,
        FLEX_CHILD(0, 1, 0, RECT(0xff0000, 60, 60)),
        FLEX_CHILD(0, 1, 0, RECT(0x0000ff, 60, 60)));
    ax_set_dimensions(s, AX_DIM(200, 200));
    ax_set_root(s, &root);
    CHECK_SZEQ(s->tree.count, (size_t) 3);
    CHECK_IEQ(N(0)->ty, AX_NODE_CONTAINER);
    CHECK_IEQ(N(1)->ty, AX_NODE_RECTANGLE);
    CHECK_IEQ(N(2)->ty, AX_NODE_RECTANGLE);
    CHECK_IEQ_HEX(N(1)->r.fill, 0xff0000);
    CHECK_IEQ_HEX(N(2)->r.fill, 0x0000ff);
}


/* Justification tests with two 60x60 rectangles, in a 200x200 window */

#define JUSTIFY_TEST_2R(_mj, _xj, _x0, _y0, _x1, _y1) do {      \
        struct ax_state* s = ax_new_state();                    \
        const struct ax_desc root =                             \
            CONT(_mj, _xj,                                      \
                 FLEX_CHILD(0, 1, 0, RECT(0xff0000, 60, 60)),   \
                 FLEX_CHILD(0, 1, 0, RECT(0x0000ff, 60, 60)));  \
        ax_set_dimensions(s, AX_DIM(200, 200));                 \
        ax_set_root(s, &root);                                  \
        CHECK_POSEQ(N(1)->coord, AX_POS(_x0, _y0));             \
        CHECK_POSEQ(N(2)->coord, AX_POS(_x1, _y1));             \
    } while(0)

TEST(main_justify_start_2r)
{ JUSTIFY_TEST_2R(AX_JUSTIFY_START, 0, 0, 0, 60, 0); }

TEST(main_justify_end_2r)
{ JUSTIFY_TEST_2R(AX_JUSTIFY_END, 0, 80, 0, 140, 0); }

TEST(main_justify_center_2r)
{ JUSTIFY_TEST_2R(AX_JUSTIFY_CENTER, 0, 40, 0, 100, 0); }

TEST(main_justify_between_2r)
{ JUSTIFY_TEST_2R(AX_JUSTIFY_BETWEEN, 0, 0, 0, 140, 0); }

TEST(main_justify_even_2r)
{ JUSTIFY_TEST_2R(AX_JUSTIFY_EVENLY, 0, 26.666, 0, 113.333, 0); }

TEST(main_justify_around_2r)
{ JUSTIFY_TEST_2R(AX_JUSTIFY_AROUND, 0, 20, 0, 120, 0); }

#undef JUSTIFY_TEST_2R
