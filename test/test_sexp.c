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
    CHECK_TRUE((C_ALPHA & C_HEX_MASK) == 0);
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


static char* sexp_readout(const char* input)
{
    char const* const inp_end = input + strlen(input);
    char* inp = (char*) input;
    char* const out_start = malloc(4096);
    char* out = out_start;
    out += sprintf(out, "{");
    struct ax_parser p;
    enum ax_parse r;
    ax__parser_init(&p);
    for (bool end = false; !end; ) {
        if (inp >= inp_end) {
            end = true;
            r = ax__parser_eof(&p);
        } else {
            r = ax__parser_feed(&p, inp, &inp);
        }
        switch (r) {
        case AX_PARSE_NOTHING: break;
        case AX_PARSE_INTEGER: out += sprintf(out, "i:%ld,", p.i); break;
        case AX_PARSE_LPAREN: out += sprintf(out, "l:%zu,", p.paren_depth); break;
        case AX_PARSE_RPAREN: out += sprintf(out, "r:%zu,", p.paren_depth); break;
        case AX_PARSE_SYMBOL: out += sprintf(out, "s:%s,", p.str); break;
        case AX_PARSE_STRING: out += sprintf(out, "S:%s,", p.str); break;
        case AX_PARSE_ERROR: out += sprintf(out, "e:%s,", p.str); break;
        default: NO_SUCH_TAG("ax_parse");
        }
    }
    if (out == out_start + 1) {
        sprintf(out, "}");
    } else {
        sprintf(out - 1, "}");
    }
    ax__parser_free(&p);
    return out_start;
}

#define CHECK_SEXP(_inp, _out) do {             \
        char* _ch_out = sexp_readout(_inp);     \
        CHECK_STREQ(_ch_out, _out);             \
        free(_ch_out); } while(0)


TEST(sexp_empty)
{ CHECK_SEXP("", "{}"); }

TEST(sexp_parens_simple)
{ CHECK_SEXP("(())", "{l:1,l:2,r:1,r:0}"); }

TEST(sexp_parens_spaces)
{ CHECK_SEXP("((  \n) )  ", "{l:1,l:2,r:1,r:0}"); }

TEST(sexp_err_bad_char)
{ CHECK_SEXP("$", "{e:invalid character `$'}"); }

TEST(sexp_err_extra_rparen)
{ CHECK_SEXP("( ))", "{l:1,r:0,e:unexpected `)'}"); }

TEST(sexp_err_unmatch_lparen)
{ CHECK_SEXP("( ()", "{l:1,l:2,r:1,e:unmatched `('}"); }

TEST(sexp_err_unmatch_quote)
{ CHECK_SEXP("\"hi\" \"world", "{S:hi,e:unmatched \"}"); }

TEST(sexp_err_bad_int)
{ CHECK_SEXP("123a", "{e:invalid character `a'}"); }

TEST(sexp_2i)
{ CHECK_SEXP("123 456", "{i:123,i:456}"); }

TEST(sexp_3s)
{ CHECK_SEXP("foo bar baz", "{s:foo,s:bar,s:baz}"); }

TEST(sexp_sym_with_digits)
{ CHECK_SEXP("foo bar123 baz", "{s:foo,s:bar123,s:baz}"); }

TEST(sexp_sym_weird)
{ CHECK_SEXP("(foo_bar-23-baz_) _hello w-orld",
             "{l:1,s:foo_bar-23-baz_,r:0,s:_hello,s:w-orld}"); }

TEST(sexp_2S_quoted)
{ CHECK_SEXP("\"hello\" \"world\"", "{S:hello,S:world}"); }

TEST(sexp_nested)
{ CHECK_SEXP("foo (bar (123 \"baz\") (45) ( )x) ",
             "{s:foo,l:1,s:bar,l:2,i:123,S:baz,r:1,"
             "l:2,i:45,r:1,l:2,r:1,s:x,r:0}"); }

#define BIG_LENGTH 100

TEST(sexp_long_sym)
{
    char inp[BIG_LENGTH + 1];
    char out[BIG_LENGTH + 5];
    memset(inp, 'A', BIG_LENGTH);
    inp[BIG_LENGTH] = '\0';
    sprintf(out, "{s:");
    memset(out + 3, 'A', BIG_LENGTH);
    out[BIG_LENGTH + 3] = '}';
    out[BIG_LENGTH + 4] = '\0';
    CHECK_SEXP(inp, out);
}

TEST(sexp_long_str)
{
    char inp[BIG_LENGTH + 3];
    char out[BIG_LENGTH + 5];
    inp[0] = '"';
    memset(inp + 1, 'A', BIG_LENGTH);
    inp[BIG_LENGTH + 1] = '"';
    inp[BIG_LENGTH + 2] = '\0';
    sprintf(out, "{S:");
    memset(out + 3, 'A', BIG_LENGTH);
    out[BIG_LENGTH + 3] = '}';
    out[BIG_LENGTH + 4] = '\0';
    CHECK_SEXP(inp, out);
}

#undef BIG_LENGTH
