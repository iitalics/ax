#include <ctype.h>
#include "sexp.h"
#include "sexp_chars.h"
#include "utils.h"

enum state {
    S_NOTHING = 0,
    S_INTEGER = 1,
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
    return AX_PARSE_ERROR;
}

static enum ax_parse ax_extra_rparen_err(struct ax_parser* p)
{
    p->len = sprintf(p->str, "unexpected `)'");
    p->err = AX_PARSE_ERROR_EXTRA_RPAREN;
    return AX_PARSE_ERROR;
}

static enum ax_parse ax_unmatch_lparen_err(struct ax_parser* p)
{
    p->len = sprintf(p->str, "unmatched `('");
    p->err = AX_PARSE_ERROR_UNMATCH_LPAREN;
    return AX_PARSE_ERROR;
}

static inline long digit(char c) { return c - '0'; }

static enum ax_parse ax_end_state(struct ax_parser* p)
{
    switch (p->state) {
    case S_NOTHING:
        return AX_PARSE_NOTHING;
    case S_INTEGER:
        p->state = S_NOTHING;
        return AX_PARSE_INTEGER;
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

enum ax_parse ax__parser_feed(struct ax_parser* p,
                              char const* chars,
                              char** out_chars)
{
    enum ax_parse rv;
    char ch;
    enum char_class cc;
    for (; (ch = *chars) != '\0'; chars++) {
        cc = ax__char_class(ch);
        if ((cc & C_DELIMIT_MASK) && p->state != S_NOTHING) {
            rv = ax_end_state(p);
            goto stop; // doesn't consume char
        }

        switch (cc) {
        case C_WHITESPACE:
            break;

        case C_LPAREN:
            p->paren_depth++;
            rv = AX_PARSE_LPAREN;
            goto consume_and_stop;

        case C_RPAREN:
            if (p->paren_depth == 0) {
                rv = ax_extra_rparen_err(p);
            } else {
                p->paren_depth--;
                rv = AX_PARSE_RPAREN;
            }
            goto consume_and_stop;

        case C_DECIMAL:
            switch (p->state) {
            case S_INTEGER:
                p->i = p->i * 10 + digit(ch);
                break;

            default:
                p->state = S_INTEGER;
                p->i = digit(ch);
                break;
            }
            break;

        default:
            rv = ax_bad_char_err(p, ch);
            goto consume_and_stop;
        }
    }
    rv = AX_PARSE_NOTHING;

stop:
    if (out_chars != NULL) {
        *out_chars = (char*) chars;
    }
    return rv;

consume_and_stop:
    chars++;
    goto stop;
}
