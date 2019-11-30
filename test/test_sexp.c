#include "helpers.h"
#include "../src/sexp.h"
#include "../src/sexp_chars.h"


TEST(sexp_chars)
{
    CHECK_TRUE((C_LPAREN & C_DELIMIT_MASK) != 0);
    CHECK_TRUE((C_RPAREN & C_DELIMIT_MASK) != 0);
    CHECK_TRUE((C_WHITESPACE & C_DELIMIT_MASK) != 0);
    CHECK_TRUE((C_HASH & C_DELIMIT_MASK) != 0);
    CHECK_TRUE((C_QUOTE & C_DELIMIT_MASK) != 0);

    CHECK_TRUE((C_ALPHA & C_SYM0_MASK) != 0);
    CHECK_TRUE((C_OTHERSYM & C_SYM0_MASK) != 0);
    CHECK_TRUE((C_DECIMAL & C_SYM0_MASK) == 0);

    CHECK_TRUE((C_ALPHA & C_SYM1_MASK) != 0);
    CHECK_TRUE((C_OTHERSYM & C_SYM1_MASK) != 0);
    CHECK_TRUE((C_DECIMAL & C_SYM1_MASK) != 0);

    CHECK_TRUE((C_DECIMAL & C_HEX_MASK) != 0);
    CHECK_TRUE((C_HEXALPHA & C_HEX_MASK) != 0);
    CHECK_TRUE((C_OTHERSYM & C_HEX_MASK) == 0);

    CHECK_IEQ_HEX(ax__char_class('('), C_LPAREN);
    CHECK_IEQ_HEX(ax__char_class(')'), C_RPAREN);
    CHECK_IEQ_HEX(ax__char_class(' '), C_WHITESPACE);
    CHECK_IEQ_HEX(ax__char_class('\t'), C_WHITESPACE);
    CHECK_IEQ_HEX(ax__char_class('\r'), C_WHITESPACE);
    CHECK_IEQ_HEX(ax__char_class('\n'), C_WHITESPACE);
    CHECK_IEQ_HEX(ax__char_class('5'), C_DECIMAL);
    CHECK_IEQ_HEX(ax__char_class('b'), C_HEXALPHA);
    CHECK_IEQ_HEX(ax__char_class('g'), C_ALPHA);
    CHECK_IEQ_HEX(ax__char_class('F'), C_HEXALPHA);
    CHECK_IEQ_HEX(ax__char_class('Y'), C_ALPHA);
    CHECK_IEQ_HEX(ax__char_class('-'), C_OTHERSYM);
    CHECK_IEQ_HEX(ax__char_class('_'), C_OTHERSYM);
    CHECK_IEQ_HEX(ax__char_class('$'), C_INVALID);
    CHECK_IEQ_HEX(ax__char_class('#'), C_HASH);
    CHECK_IEQ_HEX(ax__char_class('"'), C_QUOTE);
}


TEST(sexp_empty)
{
    struct ax_parser p;
    ax__parser_init(&p);
    CHECK_IEQ(ax__parser_eof(&p), AX_PARSE_NOTHING);
    ax__parser_free(&p);
}

TEST(sexp_parens_simple)
{
    struct ax_parser p;
    ax__parser_init(&p);
    CHECK_IEQ(ax__parser_feedc(&p, '('), AX_PARSE_LPAREN);
    CHECK_SZEQ(p.paren_depth, (size_t) 1);
    CHECK_IEQ(ax__parser_feedc(&p, '('), AX_PARSE_LPAREN);
    CHECK_SZEQ(p.paren_depth, (size_t) 2);
    CHECK_IEQ(ax__parser_feedc(&p, ')'), AX_PARSE_RPAREN);
    CHECK_SZEQ(p.paren_depth, (size_t) 1);
    CHECK_IEQ(ax__parser_feedc(&p, ')'), AX_PARSE_RPAREN);
    CHECK_SZEQ(p.paren_depth, (size_t) 0);
    CHECK_IEQ(ax__parser_eof(&p), AX_PARSE_NOTHING);
    ax__parser_free(&p);
}

TEST(sexp_parens_spaces)
{
    struct ax_parser p;
    enum ax_parse r;
    ax__parser_init(&p);
    r = ax__parser_feedc(&p, '('); CHECK_IEQ(r, AX_PARSE_LPAREN);
    CHECK_SZEQ(p.paren_depth, (size_t) 1);
    r = ax__parser_feedc(&p, '('); CHECK_IEQ(r, AX_PARSE_LPAREN);
    CHECK_SZEQ(p.paren_depth, (size_t) 2);
    r = ax__parser_feedc(&p, ' '); CHECK_IEQ(r, AX_PARSE_NOTHING);

    char s[] = " \n)";
    char* a;
    r = ax__parser_feed(&p, s, &a); CHECK_IEQ(r, AX_PARSE_RPAREN);
    CHECK_PEQ(a, &s[3]);
    CHECK_SZEQ(p.paren_depth, (size_t) 1);

    r = ax__parser_feedc(&p, ' '); CHECK_IEQ(r, AX_PARSE_NOTHING);
    r = ax__parser_feedc(&p, ')'); CHECK_IEQ(r, AX_PARSE_RPAREN);
    CHECK_SZEQ(p.paren_depth, (size_t) 0);
    r = ax__parser_feedc(&p, '\t'); CHECK_IEQ(r, AX_PARSE_NOTHING);
    r = ax__parser_feedc(&p, ' '); CHECK_IEQ(r, AX_PARSE_NOTHING);
    r = ax__parser_eof(&p); CHECK_BEQ(r, AX_PARSE_NOTHING);
    ax__parser_free(&p);
}

TEST(sexp_err_bad_char)
{
    struct ax_parser p;
    ax__parser_init(&p);
    CHECK_IEQ(ax__parser_feedc(&p, '$'), AX_PARSE_ERROR);
    CHECK_IEQ(p.err, AX_PARSE_ERROR_BAD_CHAR);
    CHECK_STREQ(p.str, "invalid character `$'");
    ax__parser_free(&p);
}

TEST(sexp_err_extra_rparen)
{
    struct ax_parser p;
    ax__parser_init(&p);
    ax__parser_feedc(&p, '(');
    ax__parser_feedc(&p, ' ');
    ax__parser_feedc(&p, ')');
    enum ax_parse r = ax__parser_feedc(&p, ')');
    CHECK_IEQ(r, AX_PARSE_ERROR);
    CHECK_IEQ(p.err, AX_PARSE_ERROR_EXTRA_RPAREN);
    CHECK_STREQ(p.str, "unexpected `)'");
    ax__parser_free(&p);
}

TEST(sexp_err_unmatch_lparen)
{
    struct ax_parser p;
    ax__parser_init(&p);
    ax__parser_feedc(&p, '(');
    ax__parser_feedc(&p, ' ');
    ax__parser_feedc(&p, '(');
    ax__parser_feedc(&p, ')');
    enum ax_parse r = ax__parser_eof(&p);
    CHECK_IEQ(r, AX_PARSE_ERROR);
    CHECK_IEQ(p.err, AX_PARSE_ERROR_UNMATCH_LPAREN);
    CHECK_STREQ(p.str, "unmatched `('");
    ax__parser_free(&p);
}
