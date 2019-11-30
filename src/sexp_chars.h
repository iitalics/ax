#pragma once


enum char_class {
    C_DELIMIT_MASK = 1,
    C_DECIMAL_MASK = 2,
    C_SYM0_MASK    = 4 | 8 | 16,
    C_SYM1_MASK    = 2 | 4 | 8 | 16,
    C_HEX_MASK     = 2 | 8,

    C_INVALID    = 0,
    C_WHITESPACE = 0x0100 | 1,
    C_LPAREN     = 0x0200 | 1,
    C_RPAREN     = 0x0300 | 1,
    C_HASH       = 0x0400 | 1,
    C_QUOTE      = 0x0500 | 1,
    C_DECIMAL    = 0x0600 | 2,
    C_ALPHA      = 0x0700 | 4,
    C_HEXALPHA   = 0x0800 | 8,
    C_OTHERSYM   = 0x0900 | 16,
    C_EOF        = 0x1000 | 1,
};

extern enum char_class ax__char_class(char c);
