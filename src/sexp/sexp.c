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

#define NO_SUCH_STATE() NO_SUCH_TAG("ax_lexer_state")

void ax__init_lexer(struct ax_lexer* lex)
{
    ax__init_region(&lex->cur_rgn);
    ax__init_region(&lex->swap_rgn);
    lex->state = S_NOTHING;
    lex->paren_depth = 0;
    lex->len = 0;
    lex->str = ALLOCATES(&lex->cur_rgn, char, lex->cap = 32);
}

void ax__free_lexer(struct ax_lexer* lex)
{
    ax__free_region(&lex->swap_rgn);
    ax__free_region(&lex->cur_rgn);
}

static void push_char(struct ax_lexer* lex, char ch)
{
    if (lex->len + 1 >= lex->cap) {
        size_t new_cap = lex->cap * 2;
        char* new_str = ALLOCATES(&lex->swap_rgn, char, new_cap);
        memcpy(new_str, lex->str, lex->len);
        lex->str = new_str;
        lex->cap = new_cap;
        ax__swap_regions(&lex->cur_rgn, &lex->swap_rgn);
        ax__region_clear(&lex->swap_rgn);
    }
    lex->str[lex->len++] = ch;
}

static void nul_term(struct ax_lexer* lex)
{
    lex->str[lex->len] = '\0';
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

static enum ax_parse bad_char_err(struct ax_lexer* lex, char c)
{
    lex->len = sprintf(lex->str, "invalid character `%c'", c);
    lex->err = AX_PARSE_ERROR_BAD_CHAR;
    lex->state = S_NOTHING;
    return AX_PARSE_ERROR;
}

static enum ax_parse extra_rparen_err(struct ax_lexer* lex)
{
    lex->len = sprintf(lex->str, "unexpected `)'");
    lex->err = AX_PARSE_ERROR_EXTRA_RPAREN;
    lex->state = S_NOTHING;
    return AX_PARSE_ERROR;
}

static enum ax_parse unmatch_lparen_err(struct ax_lexer* lex)
{
    lex->len = sprintf(lex->str, "unmatched `('");
    lex->err = AX_PARSE_ERROR_UNMATCH_LPAREN;
    lex->state = S_NOTHING;
    return AX_PARSE_ERROR;
}

static enum ax_parse unmatch_quote_err(struct ax_lexer* lex)
{
    lex->len = sprintf(lex->str, "unmatched \"");
    lex->err = AX_PARSE_ERROR_UNMATCH_QUOTE;
    lex->state = S_NOTHING;
    return AX_PARSE_ERROR;
}

static enum ax_parse end(struct ax_lexer* lex)
{
    switch (lex->state) {
    case S_NOTHING:
        return AX_PARSE_NOTHING;
    case S_INTEGER:
        lex->state = S_NOTHING;
        return AX_PARSE_INTEGER;
    case S_DOUBLE_DECPT:
        lex->state = S_NOTHING;
        lex->d += lex->i / (double) lex->dec_pt_mag;
        return AX_PARSE_DOUBLE;
    case S_SYMBOL:
        lex->state = S_NOTHING;
        nul_term(lex);
        return AX_PARSE_SYMBOL;
    case S_QUOTE_STRING:
        return unmatch_quote_err(lex);
    default: NO_SUCH_STATE();
    }
}

enum ax_parse ax__lexer_eof(struct ax_lexer* lex)
{
    if (lex->paren_depth > 0) {
        lex->paren_depth = 0;
        return unmatch_lparen_err(lex);
    }
    return end(lex);
}

static inline enum ax_parse lparen(struct ax_lexer* lex)
{
    lex->paren_depth++;
    return AX_PARSE_LPAREN;
}

static inline enum ax_parse rparen(struct ax_lexer* lex)
{
    if (lex->paren_depth == 0) {
        return extra_rparen_err(lex);
    } else {
        lex->paren_depth--;
        return AX_PARSE_RPAREN;
    }
}

static inline enum ax_parse ax_sym1(struct ax_lexer* lex, char ch)
{
    ASSERT(ax__char_class(ch) & C_SYM1_MASK, "should be SYM1");
    ASSERT(lex->state == S_SYMBOL, "should be in SYMBOL state");
    push_char(lex, ch);
    return AX_PARSE_NOTHING;
}

static inline enum ax_parse sym1(struct ax_lexer* lex, char ch)
{
    ASSERT(ax__char_class(ch) & C_SYM0_MASK, "should be SYM0");
    switch (lex->state) {
    case S_NOTHING:
        lex->state = S_SYMBOL;
        lex->len = 0;
        push_char(lex, ch);
        return AX_PARSE_NOTHING;

    case S_SYMBOL:
        ax_sym1(lex, ch);
        return AX_PARSE_NOTHING;

    case S_INTEGER:
        return bad_char_err(lex, ch);

    case S_QUOTE_STRING: ASSERT(0, "should not be reachable");
    default: NO_SUCH_STATE();
    }
}

static inline long digit(char c) { return c - '0'; }

static enum ax_parse decimal(struct ax_lexer* lex, char ch)
{
    ASSERT(ax__char_class(ch) & C_DECIMAL_MASK, "should be DEC");
    switch (lex->state) {
    case S_NOTHING:
        lex->state = S_INTEGER;
        lex->i = digit(ch);
        return AX_PARSE_NOTHING;

    case S_DOUBLE_DECPT:
        lex->dec_pt_mag *= 10;
        // fallthrough
    case S_INTEGER:
        lex->i = lex->i * 10 + digit(ch);
        return AX_PARSE_NOTHING;

    case S_SYMBOL:
        ax_sym1(lex, ch);
        return AX_PARSE_NOTHING;

    case S_QUOTE_STRING: ASSERT(0, "should not be reachable");
    default: NO_SUCH_STATE();
    }
}

static enum ax_parse quote(struct ax_lexer* lex)
{
    lex->state = S_QUOTE_STRING;
    lex->len = 0;
    return AX_PARSE_NOTHING;
}

static enum ax_parse quoted_char(struct ax_lexer* lex, char ch)
{
    ASSERT(lex->state == S_QUOTE_STRING, "should be in QUOTE_STRING state");
    if (ch == '\"') {
        lex->state = S_NOTHING;
        nul_term(lex);
        return AX_PARSE_STRING;
    } else {
        push_char(lex, ch);
        return AX_PARSE_NOTHING;
    }
}

static enum ax_parse dot(struct ax_lexer* lex)
{
    switch (lex->state) {
    case S_INTEGER:
        lex->state = S_DOUBLE_DECPT;
        lex->d = (double) lex->i;
        lex->i = 0;
        lex->dec_pt_mag = 1;
        return AX_PARSE_NOTHING;

    case S_NOTHING:
    case S_SYMBOL:
    case S_DOUBLE_DECPT:
        return bad_char_err(lex, '.');

    case S_QUOTE_STRING: ASSERT(0, "should not be reachable");
    default: NO_SUCH_STATE();
    }
}

enum ax_parse ax__lexer_feed(struct ax_lexer* lex,
                             const char* chars,
                             char** out_chars)
{
    char ch;
    enum ax_parse rv;
    while ((ch = *chars) != '\0') {
        enum ax_parse r;
        enum char_class cc = ax__char_class(ch);
        if (lex->state == S_QUOTE_STRING) {
            r = quoted_char(lex, ch);
            chars++;
        } else if ((cc & C_DELIMIT_MASK) && lex->state != S_NOTHING) {
            r = end(lex);
        }  else {
            switch (cc) {
            case C_WHITESPACE: r = AX_PARSE_NOTHING; break;
            case C_LPAREN: r = lparen(lex); break;
            case C_RPAREN: r = rparen(lex); break;
            case C_DECIMAL: r = decimal(lex, ch); break;
            case C_QUOTE: r = quote(lex); break;
            case C_DOT: r = dot(lex); break;
            case C_HASH: NOT_IMPL();
            default:
                if (cc & C_SYM0_MASK) {
                    r = sym1(lex, ch);
                } else {
                    ASSERT(cc == C_INVALID, "should be an invalid char");
                    r = bad_char_err(lex, ch);
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
