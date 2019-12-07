#include "helpers.h"
#include "../src/geom/text.h"
#include "../src/utils.h"
#include "../src/backend.h"


TEST(text_3_words)
{
    struct ax_text_iter ti;
    enum ax_text_elem e;
    ax__text_iter_init(&ti, "Foo bar, baz.");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_WORD); CHECK_STREQ(ti.word, "Foo");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_WORD); CHECK_STREQ(ti.word, "bar,");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_WORD); CHECK_STREQ(ti.word, "baz.");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_END); CHECK_STREQ(ti.line, "Foo bar, baz.");
    ax__text_iter_free(&ti);
}


TEST(text_3_words_big_spaces)
{
    struct ax_text_iter ti;
    enum ax_text_elem e;
    ax__text_iter_init(&ti, "   Foo bar,  baz.  ");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_WORD); CHECK_STREQ(ti.word, "Foo");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_WORD); CHECK_STREQ(ti.word, "bar,");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_WORD); CHECK_STREQ(ti.word, "baz.");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_END); CHECK_STREQ(ti.line, "Foo bar, baz.");
    ax__text_iter_free(&ti);
}


TEST(text_linebreak_chars)
{
    struct ax_text_iter ti;
    enum ax_text_elem e;
    ax__text_iter_init(&ti, "Foo bar,\nbaz. \n \nBang.");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_WORD); CHECK_STREQ(ti.word, "Foo");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_WORD); CHECK_STREQ(ti.word, "bar,");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_EOL); CHECK_STREQ(ti.line, "Foo bar,");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_WORD); CHECK_STREQ(ti.word, "baz.");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_EOL); CHECK_STREQ(ti.line, "baz.");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_EOL); CHECK_STREQ(ti.line, "");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_WORD); CHECK_STREQ(ti.word, "Bang.");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_END); CHECK_STREQ(ti.line, "Bang.");
    ax__text_iter_free(&ti);
}


TEST(text_linebreak_width)
{
    struct ax_text_iter ti;
    enum ax_text_elem e;
    ax__text_iter_init(&ti, "Foo bar baz bang. Superlongword.");
    ax_length font_size = 10;
    ax__text_iter_set_font(&ti, &font_size);
    ti.max_width = 80;
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_WORD); CHECK_STREQ(ti.word, "Foo");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_WORD); CHECK_STREQ(ti.word, "bar");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_EOL); CHECK_STREQ(ti.line, "Foo bar");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_WORD); CHECK_STREQ(ti.word, "baz");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_EOL); CHECK_STREQ(ti.line, "baz");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_WORD); CHECK_STREQ(ti.word, "bang.");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_EOL); CHECK_STREQ(ti.line, "bang.");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_WORD); CHECK_STREQ(ti.word, "Superlongword.");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_END); CHECK_STREQ(ti.line, "Superlongword.");
    ax__text_iter_free(&ti);
}
