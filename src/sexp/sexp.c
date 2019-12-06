#include <ctype.h>
#include "chars.h"
#include "../sexp.h"
#include "../utils.h"

enum state {
    S_NOTHING = 0,
    S_INTEGER,
    S_SYMBOL,
    S_QUOTE_STRING,
    S_DOUBLE_DECPT,
    S__MAX,
};

#define NO_SUCH_STATE() NO_SUCH_TAG("ax_parser_state")

void ax__init_parser(struct ax_parser* p)
{
    p->state = S_NOTHING;
    p->len = 0;
    p->cap = 32;
    p->paren_depth = 0;
    p->str = malloc(p->cap);
    ASSERT(p->str != NULL, "malloc ax_parser.str");
    memset(p->str, 0xff, p->cap); // try to prevent silent failure
}

void ax__free_parser(struct ax_parser* p)
{
    free(p->str);
}

static void ax_push_char(struct ax_parser* p, char ch)
{
    if (p->len + 1 >= p->cap) {
        p->cap *= 2;
        p->str = realloc(p->str, p->cap);
        ASSERT(p->str != NULL, "realloc ax_parser.str");
    }
    p->str[p->len++] = ch;
}

static void ax_nul_terminate(struct ax_parser* p)
{
    p->str[p->len] = '\0';
}


enum char_class ax__char_class(char c)
{
    switch (c) {
    case '(': return C_LPAREN;
    case ')': return C_RPAREN;
    case ' ': case '\t': case '\r': case '\n': return C_WHITESPACE;
    case '-': case '_': return C_OTHERSYM;
    case '#': return C_HASH;
    case '"': return C_QUOTE;
    case '.': return C_DOT;
    default:
        if (c >= '0' && c <= '9') {
            return C_DECIMAL;
        } else if ((c >= 'a' && c <= 'f') ||
                   (c >= 'A' && c <= 'F')) {
            return C_HEXALPHA;
        } else if ((c >= 'a' && c <= 'z') ||
                   (c >= 'A' && c <= 'Z')) {
            return C_ALPHA;
        } else {
            return C_INVALID;
        }
    }
}

static enum ax_parse ax_bad_char_err(struct ax_parser* p, char c)
{
    p->len = sprintf(p->str, "invalid character `%c'", c);
    p->err = AX_PARSE_ERROR_BAD_CHAR;
    p->state = S_NOTHING;
    return AX_PARSE_ERROR;
}

static enum ax_parse ax_extra_rparen_err(struct ax_parser* p)
{
    p->len = sprintf(p->str, "unexpected `)'");
    p->err = AX_PARSE_ERROR_EXTRA_RPAREN;
    p->state = S_NOTHING;
    return AX_PARSE_ERROR;
}

static enum ax_parse ax_unmatch_lparen_err(struct ax_parser* p)
{
    p->len = sprintf(p->str, "unmatched `('");
    p->err = AX_PARSE_ERROR_UNMATCH_LPAREN;
    p->state = S_NOTHING;
    return AX_PARSE_ERROR;
}

static enum ax_parse ax_unmatch_quote_err(struct ax_parser* p)
{
    p->len = sprintf(p->str, "unmatched \"");
    p->err = AX_PARSE_ERROR_UNMATCH_QUOTE;
    p->state = S_NOTHING;
    return AX_PARSE_ERROR;
}

static enum ax_parse ax_end_state(struct ax_parser* p)
{
    switch (p->state) {
    case S_NOTHING:
        return AX_PARSE_NOTHING;
    case S_INTEGER:
        p->state = S_NOTHING;
        return AX_PARSE_INTEGER;
    case S_DOUBLE_DECPT:
        p->state = S_NOTHING;
        p->d += p->i / (double) p->dec_pt_mag;
        return AX_PARSE_DOUBLE;
    case S_SYMBOL:
        p->state = S_NOTHING;
        ax_nul_terminate(p);
        return AX_PARSE_SYMBOL;
    case S_QUOTE_STRING:
        return ax_unmatch_quote_err(p);
    default: NO_SUCH_STATE();
    }
}

enum ax_parse ax__parser_eof(struct ax_parser* p)
{
    if (p->paren_depth > 0) {
        p->paren_depth = 0;
        return ax_unmatch_lparen_err(p);
    }
    return ax_end_state(p);
}

static inline enum ax_parse ax_lparen(struct ax_parser* p)
{
    p->paren_depth++;
    return AX_PARSE_LPAREN;
}

static inline enum ax_parse ax_rparen(struct ax_parser* p)
{
    if (p->paren_depth == 0) {
        return ax_extra_rparen_err(p);
    } else {
        p->paren_depth--;
        return AX_PARSE_RPAREN;
    }
}

static inline enum ax_parse ax_sym1(struct ax_parser* p, char ch)
{
    ASSERT(ax__char_class(ch) & C_SYM1_MASK, "should be SYM1");
    ASSERT(p->state == S_SYMBOL, "should be in SYMBOL state");
    ax_push_char(p, ch);
    return AX_PARSE_NOTHING;
}

static inline enum ax_parse ax_sym0(struct ax_parser* p, char ch)
{
    ASSERT(ax__char_class(ch) & C_SYM0_MASK, "should be SYM0");
    switch (p->state) {
    case S_NOTHING:
        p->state = S_SYMBOL;
        p->len = 0;
        ax_push_char(p, ch);
        return AX_PARSE_NOTHING;

    case S_SYMBOL:
        ax_sym1(p, ch);
        return AX_PARSE_NOTHING;

    case S_INTEGER:
        return ax_bad_char_err(p, ch);

    case S_QUOTE_STRING: ASSERT(0, "should not be reachable");
    default: NO_SUCH_STATE();
    }
}

static inline long digit(char c) { return c - '0'; }

static enum ax_parse ax_decimal(struct ax_parser* p, char ch)
{
    ASSERT(ax__char_class(ch) & C_DECIMAL_MASK, "should be DEC");
    switch (p->state) {
    case S_NOTHING:
        p->state = S_INTEGER;
        p->i = digit(ch);
        return AX_PARSE_NOTHING;

    case S_DOUBLE_DECPT:
        p->dec_pt_mag *= 10;
    case S_INTEGER:
        p->i = p->i * 10 + digit(ch);
        return AX_PARSE_NOTHING;

    case S_SYMBOL:
        ax_sym1(p, ch);
        return AX_PARSE_NOTHING;

    case S_QUOTE_STRING: ASSERT(0, "should not be reachable");
    default: NO_SUCH_STATE();
    }
}

static enum ax_parse ax_quote(struct ax_parser* p)
{
    p->state = S_QUOTE_STRING;
    p->len = 0;
    return AX_PARSE_NOTHING;
}

static enum ax_parse ax_quoted_char(struct ax_parser* p, char ch)
{
    ASSERT(p->state == S_QUOTE_STRING, "should be in QUOTE_STRING state");
    if (ch == '\"') {
        p->state = S_NOTHING;
        ax_nul_terminate(p);
        return AX_PARSE_STRING;
    } else {
        ax_push_char(p, ch);
        return AX_PARSE_NOTHING;
    }
}

static enum ax_parse ax_dot(struct ax_parser* p)
{
    switch (p->state) {
    case S_INTEGER:
        p->state = S_DOUBLE_DECPT;
        p->d = (double) p->i;
        p->i = 0;
        p->dec_pt_mag = 1;
        return AX_PARSE_NOTHING;

    case S_NOTHING:
    case S_SYMBOL:
    case S_DOUBLE_DECPT:
        return ax_bad_char_err(p, '.');

    case S_QUOTE_STRING: ASSERT(0, "should not be reachable");
    default: NO_SUCH_STATE();
    }
}

enum ax_parse ax__parser_feed(struct ax_parser* p,
                              char const* chars,
                              char** out_chars)
{
    char ch;
    enum ax_parse rv;
    while ((ch = *chars) != '\0') {
        enum ax_parse r;
        enum char_class cc = ax__char_class(ch);
        if (p->state == S_QUOTE_STRING) {
            r = ax_quoted_char(p, ch);
            chars++;
        } else if ((cc & C_DELIMIT_MASK) && p->state != S_NOTHING) {
            r = ax_end_state(p);
        }  else {
            switch (cc) {
            case C_WHITESPACE: r = AX_PARSE_NOTHING; break;
            case C_LPAREN: r = ax_lparen(p); break;
            case C_RPAREN: r = ax_rparen(p); break;
            case C_DECIMAL: r = ax_decimal(p, ch); break;
            case C_QUOTE: r = ax_quote(p); break;
            case C_DOT: r = ax_dot(p); break;
            case C_HASH: NOT_IMPL();
            default:
                if (cc & C_SYM0_MASK) {
                    r = ax_sym0(p, ch);
                } else {
                    ASSERT(cc == C_INVALID, "should be an invalid char");
                    r = ax_bad_char_err(p, ch);
                }
                break;
            }
            chars++;
        }

        if (r != AX_PARSE_NOTHING) {
            rv = r;
            goto stop;
        }
    }

    rv = AX_PARSE_NOTHING;
stop:
    if (out_chars != NULL) {
        *out_chars = (char*) chars;
    }
    return rv;
}
