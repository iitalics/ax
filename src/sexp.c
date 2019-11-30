#include <ctype.h>
#include "sexp.h"
#include "sexp_chars.h"
#include "utils.h"

enum state {
    S_NOTHING = 0,
    S_INTEGER,
    S_SYMBOL,
    S__MAX,
};

#define NO_SUCH_STATE() NO_SUCH_TAG("ax_parser_state")

void ax__parser_init(struct ax_parser* p)
{
    p->state = S_NOTHING;
    p->str = malloc(32);
    ASSERT(p->str != NULL, "malloc ax_parser.str");
    p->str[0] = '\0';
    p->len = 0;
    p->cap = 32;
    p->paren_depth = 0;
}

void ax__parser_free(struct ax_parser* p)
{
    free(p->str);
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

static enum ax_parse ax_end_state(struct ax_parser* p)
{
    switch (p->state) {
    case S_NOTHING:
        return AX_PARSE_NOTHING;
    case S_INTEGER:
        p->state = S_NOTHING;
        return AX_PARSE_INTEGER;
    case S_SYMBOL:
        p->state = S_NOTHING;
        return AX_PARSE_SYMBOL;
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
    ASSERT(p->len + 1 < p->cap, "symbol too big!");
    p->str[p->len++] = ch;
    p->str[p->len] = '\0';
    return AX_PARSE_NOTHING;
}

static inline enum ax_parse ax_sym0(struct ax_parser* p, char ch)
{
    ASSERT(ax__char_class(ch) & C_SYM0_MASK, "should be SYM0");
    switch (p->state) {
    case S_NOTHING:
        p->state = S_SYMBOL;
        p->len = 1;
        p->str[0] = ch;
        p->str[1] = '\0';
        return AX_PARSE_NOTHING;

    case S_SYMBOL:
        ax_sym1(p, ch);
        return AX_PARSE_NOTHING;

    case S_INTEGER:
        return ax_bad_char_err(p, ch);

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

    case S_INTEGER:
        p->i = p->i * 10 + digit(ch);
        return AX_PARSE_NOTHING;

    case S_SYMBOL:
        ax_sym1(p, ch);
        return AX_PARSE_NOTHING;

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
        if ((cc & C_DELIMIT_MASK) && p->state != S_NOTHING) {
            r = ax_end_state(p);
        } else {
            switch (cc) {
            case C_WHITESPACE: r = AX_PARSE_NOTHING; break;
            case C_LPAREN: r = ax_lparen(p); break;
            case C_RPAREN: r = ax_rparen(p); break;
            case C_DECIMAL: r = ax_decimal(p, ch); break;
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
