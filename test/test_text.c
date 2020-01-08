#include "helpers.h"
#include "../src/geom/text.h"
#include "../src/utils.h"
#include "../src/backend.h"
#include "../src/core.h"
#include "../src/core/region.h"

TEST(text_3_words)
{
    struct region rgn;
    struct ax_text_iter ti;
    enum ax_text_elem e;
    ax__init_region(&rgn);
    ax__text_iter_init(&rgn, &ti, "Foo bar, baz.");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_WORD); CHECK_STREQ(ti.word, "Foo");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_WORD); CHECK_STREQ(ti.word, "bar,");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_WORD); CHECK_STREQ(ti.word, "baz.");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_END); CHECK_STREQ(ti.line, "Foo bar, baz.");
    ax__free_region(&rgn);
}


TEST(text_3_words_big_spaces)
{
    struct region rgn;
    struct ax_text_iter ti;
    enum ax_text_elem e;
    ax__init_region(&rgn);
    ax__text_iter_init(&rgn, &ti, "   Foo bar,  baz.  ");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_WORD); CHECK_STREQ(ti.word, "Foo");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_WORD); CHECK_STREQ(ti.word, "bar,");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_WORD); CHECK_STREQ(ti.word, "baz.");
    e = ax__text_iter_next(&ti);
    CHECK_IEQ(e, AX_TEXT_END); CHECK_STREQ(ti.line, "Foo bar, baz.");
    ax__free_region(&rgn);
}


TEST(text_linebreak_chars)
{
    struct region rgn;
    struct ax_text_iter ti;
    enum ax_text_elem e;
    ax__init_region(&rgn);
    ax__text_iter_init(&rgn, &ti, "Foo bar,\nbaz. \n \nBang.");
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
    ax__free_region(&rgn);
}


TEST(text_linebreak_width)
{
    struct ax_state* s = ax_new_state();
    ax_write(s, "(init)");
    struct ax_font* f;
    ax__new_font(s, s->backend, "size:10", &f);
    struct ax_text_iter ti;
    enum ax_text_elem e;
    ax__text_iter_init(&s->init_rgn, &ti, "Foo bar baz bang. Superlongword.");
    ax__text_iter_set_font(&ti, f);
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
    ax_destroy_state(s);
}
