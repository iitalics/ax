#include "helpers.h"
#include "../src/text.h"


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


static ax_length mono_measure_fn(const char* str, void* ud)
{
    ax_length font_size = *(ax_length*) ud;
    return strlen(str) * font_size;
}

void ax__measure_text(
    void* ud,
    const char* text,
    struct ax_text_metrics* tm)
{
    ax_length font_size = *(ax_length*) ud;
    tm->line_spacing = tm->text_height = font_size;
    tm->width = mono_measure_fn(text, ud);
}


TEST(text_linebreak_width)
{
    struct ax_text_iter ti;
    enum ax_text_elem e;
    ax__text_iter_init(&ti, "Foo bar baz bang. Superlongword.");
    ax_length font_size = 10;
    ti.mf = mono_measure_fn;
    ti.mf_userdata = &font_size;
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
