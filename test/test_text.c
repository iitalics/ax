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
