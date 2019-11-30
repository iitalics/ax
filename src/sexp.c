#include <ctype.h>
#include "sexp.h"
#include "sexp_chars.h"
#include "utils.h"

enum state {
    S_WHITESPACE = 0,
    S__MAX,
};

#define NO_SUCH_STATE() NO_SUCH_TAG("ax_parser_state")

void ax__parser_init(struct ax_parser* p)
{
    p->state = S_WHITESPACE;
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
    case '\0': return C_EOF;
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

enum ax_parse ax__parser_feed(struct ax_parser* p,
                              char const* chars,
                              char** out_chars)
{
    enum ax_parse rv;
    for (;; chars++) {
        switch (p->state) {

        case S_WHITESPACE: {
            enum char_class cc = ax__char_class(*chars);
            if (cc == C_LPAREN) {
                p->paren_depth++;
                rv = AX_PARSE_LPAREN;
            } else if (cc == C_RPAREN) {
                if (p->paren_depth == 0) {
                    rv = ax_extra_rparen_err(p);
                } else {
                    p->paren_depth--;
                    rv = AX_PARSE_RPAREN;
                }
            } else if (cc == C_WHITESPACE || cc == C_EOF) {
                rv = AX_PARSE_NOTHING;
            } else {
                rv = ax_bad_char_err(p, *chars);
            }
            goto consume_and_stop;
        }

        default: NO_SUCH_STATE();
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
