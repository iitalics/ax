#include "helpers.h"
#include "../src/ax.h"
#include "../src/utils.h"


#define D(_idx) d->data[_idx]


/*TEST(color_to_rgb)
{
    uint8_t rgb[3];
    memset(rgb, 0, 3);
    ax_color_rgb(0x123456, rgb);
    CHECK_IEQ_HEX(rgb[0], 0x12);
    CHECK_IEQ_HEX(rgb[1], 0x34);
    CHECK_IEQ_HEX(rgb[2], 0x56);

    memset(rgb, 0, 3);
    ax_color_rgb(0x98123456, rgb);
    CHECK_IEQ_HEX(rgb[0], 0x12);
    CHECK_IEQ_HEX(rgb[1], 0x34);
    CHECK_IEQ_HEX(rgb[2], 0x56);
} */

/*TEST(rgb_to_color)
{
    uint8_t rgb[3] = { 0x12, 0x34, 0x56 };
    CHECK_IEQ_HEX(ax_rgb_color(rgb), 0x123456);
} */

/*TEST(draw_1r)
{
    struct ax_state* s = ax_new_state();
    ax_set_dimensions(s, AX_DIM(200, 200));
    ax_set_root(
        s,
        &(struct ax_desc) RECT(0xff0033, 60, 80));

    const struct ax_drawbuf* d = ax_draw(s);

    CHECK_SZEQ(d->len, (size_t) 1);
    CHECK_IEQ(D(0).ty, AX_DRAW_RECT);
    CHECK_IEQ(D(0).r.fill, 0xff0033);
    CHECK_POSEQ(D(0).r.bounds.o, AX_POS(0.0, 0.0));
    CHECK_DIMEQ(D(0).r.bounds.s, AX_DIM(60.0, 80.0));

    ax_destroy_state(s);
} */

/*TEST(draw_3r)
{
    struct ax_state* s = ax_new_state();
    ax_set_dimensions(s, AX_DIM(200, 200));
    ax_set_root(
        s,
        &(struct ax_desc)
        CONT(AX_JUSTIFY_BETWEEN, AX_JUSTIFY_CENTER,
             FLEX_CHILD(0, 1, AX_JUSTIFY_START, RECT(0xff0000, 60, 60)),
             FLEX_CHILD(0, 1, AX_JUSTIFY_START, RECT(0x00ff00, 20, 20)),
             FLEX_CHILD(0, 1, AX_JUSTIFY_START, RECT(0x0000ff, 60, 60)))
        );

    const struct ax_drawbuf* d = ax_draw(s);

    //
    //   +--------------+
    //   |              |
    //   |RRRR  GG  BBBB|
    //   |RRRR      BBBB|
    //   |RRRR      BBBB|
    //   |              |
    //   +--------------+
    //
    //   (see `./reference_draw_3r.html`)
    //
    CHECK_SZEQ(d->len, (size_t) 3);

    CHECK_IEQ(D(0).ty, AX_DRAW_RECT);
    CHECK_IEQ_HEX(D(0).r.fill, 0xff0000);
    CHECK_POSEQ(D(0).r.bounds.o, AX_POS(0.0, 70.0));
    CHECK_DIMEQ(D(0).r.bounds.s, AX_DIM(60.0, 60.0));

    CHECK_IEQ(D(1).ty, AX_DRAW_RECT);
    CHECK_IEQ_HEX(D(1).r.fill, 0x00ff00);
    CHECK_POSEQ(D(1).r.bounds.o, AX_POS(90.0, 70.0));
    CHECK_DIMEQ(D(1).r.bounds.s, AX_DIM(20.0, 20.0));

    CHECK_IEQ(D(2).ty, AX_DRAW_RECT);
    CHECK_IEQ_HEX(D(2).r.fill, 0x0000ff);
    CHECK_POSEQ(D(2).r.bounds.o, AX_POS(140.0, 70.0));
    CHECK_DIMEQ(D(2).r.bounds.s, AX_DIM(60.0, 60.0));

    ax_destroy_state(s);
} */

/*TEST(draw_text_1l)
{
    ax_length font_size = 10;
    struct ax_state* s = ax_new_state();
    ax_set_dimensions(s, AX_DIM(200.0, 200.0));
    ax_set_root(
        s,
        &(struct ax_desc)
        TEXT(0x111111, "Hello, world", &font_size));

    const struct ax_drawbuf* d = ax_draw(s);
    CHECK_SZEQ(d->len, (size_t) 1);

    CHECK_IEQ(D(0).ty, AX_DRAW_TEXT);
    CHECK_IEQ_HEX(D(0).t.color, 0x111111);
    CHECK_POSEQ(D(0).t.pos, AX_POS(0.0, 0.0));
    CHECK_STREQ(D(0).t.text, "Hello, world");
    CHECK_PEQ(D(0).t.font, &font_size);

    ax_destroy_state(s);
} */

/*TEST(draw_text_2l)
{
    ax_length font_size = 10;
    struct ax_state* s = ax_new_state();
    ax_set_dimensions(s, AX_DIM(100.0, 100.0));
    ax_set_root(
        s,
        &(struct ax_desc)
        TEXT(0x111111, "Hello, world", &font_size));

    const struct ax_drawbuf* d = ax_draw(s);
    CHECK_SZEQ(d->len, (size_t) 2);

    CHECK_IEQ(D(0).ty, AX_DRAW_TEXT);
    CHECK_IEQ_HEX(D(0).t.color, 0x111111);
    CHECK_POSEQ(D(0).t.pos, AX_POS(0.0, 0.0));
    CHECK_STREQ(D(0).t.text, "Hello,");
    CHECK_PEQ(D(0).t.font, &font_size);

    CHECK_IEQ(D(1).ty, AX_DRAW_TEXT);
    CHECK_IEQ_HEX(D(1).t.color, 0x111111);
    CHECK_POSEQ(D(1).t.pos, AX_POS(0.0, 10.0));
    CHECK_STREQ(D(1).t.text, "world");
    CHECK_PEQ(D(1).t.font, &font_size);

    ax_destroy_state(s);
} */
