#include "helpers.h"
#include "../src/sexp.h"


TEST(sexp_empty)
{
    struct ax_parser p;
    ax__parser_init(&p);
    CHECK_BEQ(ax__parser_eof_ok(&p), true);
    ax__parser_free(&p);
}

TEST(sexp_parens_simple)
{
    struct ax_parser p;
    ax__parser_init(&p);
    CHECK_IEQ(ax__parse_feedc(&p, '('), AX_PARSE_LPAREN);
    CHECK_SZEQ(p.paren_depth, (size_t) 1);
    CHECK_IEQ(ax__parse_feedc(&p, '('), AX_PARSE_LPAREN);
    CHECK_SZEQ(p.paren_depth, (size_t) 2);
    CHECK_BEQ(ax__parser_eof_ok(&p), false);
    CHECK_IEQ(ax__parse_feedc(&p, ')'), AX_PARSE_RPAREN);
    CHECK_SZEQ(p.paren_depth, (size_t) 1);
    CHECK_IEQ(ax__parse_feedc(&p, ')'), AX_PARSE_RPAREN);
    CHECK_SZEQ(p.paren_depth, (size_t) 0);
    CHECK_BEQ(ax__parser_eof_ok(&p), true);
    ax__parser_free(&p);
}

TEST(sexp_parens_spaces)
{
    struct ax_parser p;
    ax__parser_init(&p);
    CHECK_IEQ(ax__parse_feedc(&p, '('), AX_PARSE_LPAREN);
    CHECK_SZEQ(p.paren_depth, (size_t) 1);
    CHECK_IEQ(ax__parse_feedc(&p, '('), AX_PARSE_LPAREN);
    CHECK_SZEQ(p.paren_depth, (size_t) 2);
    CHECK_IEQ(ax__parse_feedc(&p, ' '), AX_PARSE_NOTHING);
    CHECK_BEQ(ax__parser_eof_ok(&p), false);
    CHECK_IEQ(ax__parse_feedc(&p, ')'), AX_PARSE_RPAREN);
    CHECK_IEQ(ax__parse_feedc(&p, ' '), AX_PARSE_NOTHING);
    CHECK_IEQ(ax__parse_feedc(&p, '\n'), AX_PARSE_NOTHING);
    CHECK_SZEQ(p.paren_depth, (size_t) 1);
    CHECK_IEQ(ax__parse_feedc(&p, ')'), AX_PARSE_RPAREN);
    CHECK_SZEQ(p.paren_depth, (size_t) 0);
    CHECK_IEQ(ax__parse_feedc(&p, '\t'), AX_PARSE_NOTHING);
    CHECK_IEQ(ax__parse_feedc(&p, ' '), AX_PARSE_NOTHING);
    CHECK_BEQ(ax__parser_eof_ok(&p), true);
    ax__parser_free(&p);
}

TEST(sexp_err_bad_char)
{
    struct ax_parser p;
    ax__parser_init(&p);
    CHECK_IEQ(ax__parse_feedc(&p, '$'), AX_PARSE_ERROR);
    CHECK_IEQ(p.err, AX_PARSE_ERROR_BAD_CHAR);
    CHECK_STREQ(p.str, "invalid character `$'");
    ax__parser_free(&p);
}

TEST(sexp_err_extra_rparen)
{
    struct ax_parser p;
    ax__parser_init(&p);
    ax__parse_feedc(&p, '(');
    ax__parse_feedc(&p, ' ');
    ax__parse_feedc(&p, ')');
    CHECK_IEQ(ax__parse_feedc(&p, ')'), AX_PARSE_ERROR);
    CHECK_IEQ(p.err, AX_PARSE_ERROR_EXTRA_RPAREN);
    CHECK_STREQ(p.str, "unexpected `)'");
    ax__parser_free(&p);
}
