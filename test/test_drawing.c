#include "helpers.h"
#include "../src/ax.h"
#include "../src/core.h"
#include "../src/draw.h"
#include "../src/utils.h"

#define D(_idx) s->draw_buf->data[_idx]
#define D_LEN() s->draw_buf->len

TEST(color_to_rgb)
{
    uint8_t rgb[3];
    memset(rgb, 0, 3);
    CHECK_TRUE(ax_color_to_rgb(0x123456, rgb));
    CHECK_IEQ_HEX(rgb[0], 0x12);
    CHECK_IEQ_HEX(rgb[1], 0x34);
    CHECK_IEQ_HEX(rgb[2], 0x56);
    CHECK_TRUE(ax_color_to_rgb(0xffffff, rgb));
    CHECK_IEQ_HEX(rgb[0], 0xff);
    CHECK_IEQ_HEX(rgb[1], 0xff);
    CHECK_IEQ_HEX(rgb[2], 0xff);

    CHECK_FALSE(ax_color_to_rgb(AX_NULL_COLOR, rgb));
    CHECK_FALSE(ax_color_to_rgb(0x8123456, rgb));
}

TEST(color_from_rgb)
{
    uint8_t rgb[3] = { 0x12, 0x34, 0x56 };
    CHECK_IEQ_HEX(ax_color_from_rgb(rgb), 0x123456);
}

TEST(draw_1r)
{
    struct ax_state* s = ax_new_state();
    ax_write(s,
             "(init (window-size 200 200))"
             "(set-root (rect (fill \"ff0033\") (size 60 80)))");

    CHECK_SZEQ(D_LEN(), (size_t) 1);
    CHECK_IEQ(D(0).ty, AX_DRAW_RECT);
    CHECK_IEQ(D(0).r.fill, 0xff0033);
    CHECK_POSEQ(D(0).r.bounds.o, AX_POS(0.0, 0.0));
    CHECK_DIMEQ(D(0).r.bounds.s, AX_DIM(60.0, 80.0));
    ax_destroy_state(s);
}

TEST(draw_3r)
{
    struct ax_state* s = ax_new_state();
    ax_write(s,
             "(init (window-size 200 200))"
             "(set-root"
             " (container (children (rect (fill \"ff0000\") (size 60 60))"
             "                      (rect (fill \"00ff00\") (size 20 20))"
             "                      (rect (fill \"0000ff\") (size 60 60)))"
             "            (main-justify between)"
             "            (cross-justify center)))");
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
    CHECK_SZEQ(D_LEN(), (size_t) 3);
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
}

TEST(draw_3r_colors)
{
    struct ax_state* s = ax_new_state();
    ax_write(s,
             "(init)"
             "(set-root"
             " (container (children (rect (fill \"123456\"))"
             "                      (rect (fill none))"
             "                      (rect (fill (rgb 100 200 50))))))");

    CHECK_SZEQ(D_LEN(), (size_t) 3);
    CHECK_IEQ_HEX(D(0).r.fill, 0x123456);
    CHECK_TRUE(AX_COLOR_IS_NULL(D(1).r.fill));
    CHECK_IEQ_HEX(D(2).r.fill, 0x64c832);
}

TEST(draw_text_1l)
{
    struct ax_state* s = ax_new_state();
    ax_write(s,
             "(init (window-size 200 200))"
             "(set-root (text \"Hello, world\""
             "                (color \"111111\")"
             "                (font \"size:10\")))");

    CHECK_SZEQ(D_LEN(), (size_t) 1);
    CHECK_IEQ(D(0).ty, AX_DRAW_TEXT);
    CHECK_IEQ_HEX(D(0).t.color, 0x111111);
    CHECK_POSEQ(D(0).t.pos, AX_POS(0.0, 0.0));
    CHECK_STREQ(D(0).t.text, "Hello, world");
    CHECK_FLEQ(0.0001, *(ax_length*) D(0).t.font, 10.0);
    ax_destroy_state(s);
}

TEST(draw_text_2l)
{
    struct ax_state* s = ax_new_state();
    ax_write(s,
             "(init (window-size 100 100))"
             "(set-root (text \"Hello, world\""
             "                (color \"111111\")"
             "                (font \"size:10\")))");

    CHECK_SZEQ(D_LEN(), (size_t) 2);
    CHECK_IEQ(D(0).ty, AX_DRAW_TEXT);
    CHECK_IEQ_HEX(D(0).t.color, 0x111111);
    CHECK_POSEQ(D(0).t.pos, AX_POS(0.0, 0.0));
    CHECK_STREQ(D(0).t.text, "Hello,");
    CHECK_FLEQ(0.0001, *(ax_length*) D(0).t.font, 10.0);
    CHECK_IEQ(D(1).ty, AX_DRAW_TEXT);
    CHECK_IEQ_HEX(D(1).t.color, 0x111111);
    CHECK_POSEQ(D(1).t.pos, AX_POS(0.0, 10.0));
    CHECK_STREQ(D(1).t.text, "world");
    CHECK_FLEQ(0.0001, *(ax_length*) D(1).t.font, 10.0);
    ax_destroy_state(s);
}


TEST(draw_2r_bg)
{
    struct ax_state* s = ax_new_state();
    ax_write(s,
            "(init (window-size 200 200))"
            "(set-root"
            " (container (children (rect (fill \"ff0000\") (size 60 60))"
            "                      (rect (fill \"0000ff\") (size 60 60)))"
            "            (background \"00ff00\")))");
    CHECK_SZEQ(D_LEN(), (size_t) 3);
    // container
    CHECK_IEQ(D(0).ty, AX_DRAW_RECT);
    CHECK_IEQ_HEX(D(0).r.fill, 0x00ff00);
    CHECK_POSEQ(D(0).r.bounds.o, AX_POS(0.0, 0.0));
    CHECK_DIMEQ(D(0).r.bounds.s, AX_DIM(200.0, 200.0));
    // red rect
    CHECK_IEQ(D(1).ty, AX_DRAW_RECT);
    CHECK_IEQ_HEX(D(1).r.fill, 0xff0000);
    CHECK_POSEQ(D(1).r.bounds.o, AX_POS(0.0, 0.0));
    CHECK_DIMEQ(D(1).r.bounds.s, AX_DIM(60.0, 60.0));
    // blue rect
    CHECK_IEQ(D(2).ty, AX_DRAW_RECT);
    CHECK_IEQ_HEX(D(2).r.fill, 0x0000ff);
    CHECK_POSEQ(D(2).r.bounds.o, AX_POS(60.0, 0.0));
    CHECK_DIMEQ(D(2).r.bounds.s, AX_DIM(60.0, 60.0));
    ax_destroy_state(s);
}

TEST(draw_nested_bg)
{
    struct ax_state* s = ax_new_state();
    ax_write(s,
             "(init (window-size 200 200))"
             "(set-root"
             " (container (children (container"
             "                       (children (rect (fill \"ff0000\") (size 60 60))"
             "                                 (rect (fill \"00ff00\") (size 60 20)))"
             "                       (background \"ff00ff\"))"
             "                      (rect (fill \"0000ff\") (size 60 60)))"
             "            (background \"ffff00\")))");
    CHECK_SZEQ(D_LEN(), (size_t) 5);
    // outer container
    CHECK_IEQ(D(0).ty, AX_DRAW_RECT);
    CHECK_IEQ_HEX(D(0).r.fill, 0xffff00);
    CHECK_POSEQ(D(0).r.bounds.o, AX_POS(0.0, 0.0));
    CHECK_DIMEQ(D(0).r.bounds.s, AX_DIM(200.0, 200.0));
    // inner container
    CHECK_IEQ(D(1).ty, AX_DRAW_RECT);
    CHECK_IEQ_HEX(D(1).r.fill, 0xff00ff);
    CHECK_POSEQ(D(1).r.bounds.o, AX_POS(0.0, 0.0));
    CHECK_DIMEQ(D(1).r.bounds.s, AX_DIM(120.0, 60.0));
    // red rect
    CHECK_IEQ(D(2).ty, AX_DRAW_RECT);
    CHECK_IEQ_HEX(D(2).r.fill, 0xff0000);
    CHECK_POSEQ(D(2).r.bounds.o, AX_POS(0.0, 0.0));
    CHECK_DIMEQ(D(2).r.bounds.s, AX_DIM(60.0, 60.0));
    // green rect
    CHECK_IEQ(D(3).ty, AX_DRAW_RECT);
    CHECK_IEQ_HEX(D(3).r.fill, 0x00ff00);
    CHECK_POSEQ(D(3).r.bounds.o, AX_POS(60.0, 0.0));
    CHECK_DIMEQ(D(3).r.bounds.s, AX_DIM(60.0, 20.0));
    // blue rect
    CHECK_IEQ(D(4).ty, AX_DRAW_RECT);
    CHECK_IEQ_HEX(D(4).r.fill, 0x0000ff);
    CHECK_POSEQ(D(4).r.bounds.o, AX_POS(120.0, 0.0));
    CHECK_DIMEQ(D(4).r.bounds.s, AX_DIM(60.0, 60.0));
    ax_destroy_state(s);
}

TEST(draw_nested_2_bg)
{
    struct ax_state* s = ax_new_state();
    ax_write(s,
             "(init (window-size 200 200))"
             "(set-root"    // blue rect is before inner container
             " (container (children (rect (fill \"0000ff\") (size 60 60))"
             "                      (container"
             "                       (children (rect (fill \"ff0000\") (size 60 60))"
             "                                 (rect (fill \"00ff00\") (size 60 20)))"
             "                       (background \"ff00ff\")))"
             "            (background \"ffff00\")))");
    CHECK_SZEQ(D_LEN(), (size_t) 5);
    // outer container
    CHECK_IEQ(D(0).ty, AX_DRAW_RECT);
    CHECK_IEQ_HEX(D(0).r.fill, 0xffff00);
    CHECK_POSEQ(D(0).r.bounds.o, AX_POS(0.0, 0.0));
    CHECK_DIMEQ(D(0).r.bounds.s, AX_DIM(200.0, 200.0));
    // blue rect
    CHECK_IEQ(D(1).ty, AX_DRAW_RECT);
    CHECK_IEQ_HEX(D(1).r.fill, 0x0000ff);
    CHECK_POSEQ(D(1).r.bounds.o, AX_POS(0.0, 0.0));
    CHECK_DIMEQ(D(1).r.bounds.s, AX_DIM(60.0, 60.0));
    // inner container
    CHECK_IEQ(D(2).ty, AX_DRAW_RECT);
    CHECK_IEQ_HEX(D(2).r.fill, 0xff00ff);
    CHECK_POSEQ(D(2).r.bounds.o, AX_POS(60.0, 0.0));
    CHECK_DIMEQ(D(2).r.bounds.s, AX_DIM(120.0, 60.0));
    // red rect
    CHECK_IEQ(D(3).ty, AX_DRAW_RECT);
    CHECK_IEQ_HEX(D(3).r.fill, 0xff0000);
    CHECK_POSEQ(D(3).r.bounds.o, AX_POS(60.0, 0.0));
    CHECK_DIMEQ(D(3).r.bounds.s, AX_DIM(60.0, 60.0));
    // green rect
    CHECK_IEQ(D(4).ty, AX_DRAW_RECT);
    CHECK_IEQ_HEX(D(4).r.fill, 0x00ff00);
    CHECK_POSEQ(D(4).r.bounds.o, AX_POS(120.0, 0.0));
    CHECK_DIMEQ(D(4).r.bounds.s, AX_DIM(60.0, 20.0));
    ax_destroy_state(s);
}
