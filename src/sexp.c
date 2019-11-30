#include "sexp.h"
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

bool ax__parser_eof_ok(struct ax_parser* p)
{
    switch (p->state) {
    case S_WHITESPACE:
        return p->paren_depth == 0;

    default: NO_SUCH_STATE();
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
                              char const* chars, size_t len,
                              char** out_chars)
{
    enum ax_parse rv;
    size_t i;

    for (i = 0; i < len; i++) {
        char c = chars[i];
        switch (p->state) {
        case S_WHITESPACE:
            switch (c) {
            case '(':
                p->paren_depth++;
                rv = AX_PARSE_LPAREN;
                goto stop;

            case ')':
                if (p->paren_depth == 0) {
                    rv = ax_extra_rparen_err(p);
                    goto stop;
                } else {
                    p->paren_depth--;
                    rv = AX_PARSE_RPAREN;
                    goto stop;
                }

            case ' ': case '\t': case '\f': case '\r': case '\n':
                break;

            default:
                rv = ax_bad_char_err(p, c);
                goto stop;
            }
            break;

        default: NO_SUCH_STATE();
        }
    }

    rv = AX_PARSE_NOTHING;
stop:
    if (out_chars != NULL) {
        *out_chars = (char*) &chars[i];
    }
    return rv;
}
