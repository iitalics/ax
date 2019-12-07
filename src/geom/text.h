#pragma once
#include "../base.h"

typedef ax_length (*ax_text_measure_fn)(const char*, void*);

struct ax_text_iter {
    const char* text;
    char* word;
    char* line;
    size_t line_len;
    bool line_need_reset;

    ax_text_measure_fn mf;
    void* mf_userdata;
    ax_length max_width;
};

enum ax_text_elem {
    AX_TEXT_END = 0,
    AX_TEXT_WORD,
    AX_TEXT_EOL,
    AX_TEXT__MAX
};

struct ax_text_metrics {
    ax_length width;
    ax_length text_height;
    ax_length line_spacing;
};

extern void ax__text_iter_init(struct ax_text_iter* ti, const char* text);
extern void ax__text_iter_free(struct ax_text_iter* ti);
extern void ax__text_iter_set_font(struct ax_text_iter* ti, void* font);

extern enum ax_text_elem ax__text_iter_next(struct ax_text_iter* ti);