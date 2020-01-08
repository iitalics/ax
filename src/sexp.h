#pragma once
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "core/region.h"
#include "utils.h"

/*
 * SEXP = NUM
 *      | SYM
 *      | STR
 *      | '(' SEXP* ')'
 *
 * NUM = DEC+             integer (decimal)
 *     | DEC+.DEC*        double (decimal)
 *     | '#d' HEX{8}      double (hex encoded)        TODO
 *
 * SYM = SYM0 SYM1*
 *
 * STR = '"' [^"]* '"'    string (quoted)
 *     | '#s' (HEX HEX)*  string (hex encoded)        TODO
 *
 * DEC = [0-9]
 * HEX = [0-9a-fA-F]
 * SYM0 = [-a-zA-Z_]
 * SYM1 = [-a-zA-Z_0-9]
 */

enum ax_parse {
    AX_PARSE_NOTHING = 0,
    AX_PARSE_ERROR,
    AX_PARSE_LPAREN,
    AX_PARSE_RPAREN,
    AX_PARSE_INTEGER,
    AX_PARSE_DOUBLE,
    AX_PARSE_SYMBOL,
    AX_PARSE_STRING,
    AX_PARSE__MAX,
};

enum ax_parse_error {
    AX_PARSE_ERROR_BAD_CHAR = 0,
    AX_PARSE_ERROR_EXTRA_RPAREN,
    AX_PARSE_ERROR_UNMATCH_LPAREN,
    AX_PARSE_ERROR_UNMATCH_QUOTE,
    AX_PARSE_ERROR__MAX,
};

struct ax_lexer {
    struct region cur_rgn, swap_rgn;
    int state;
    size_t paren_depth;
    size_t len, cap;
    char* str;
    enum ax_parse_error err;

    long i;
    double d;
    long dec_pt_mag;
};


void ax__init_lexer(struct ax_lexer* lex);
void ax__free_lexer(struct ax_lexer* lex);

enum ax_parse ax__lexer_feed(struct ax_lexer* lex,
                             const char* chars,
                             char** out_chars);

enum ax_parse ax__lexer_eof(struct ax_lexer* lex);
