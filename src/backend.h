#pragma once
#include "ax.h"
#include "geom/text.h" // TODO: don't depend on text.h

struct ax_backend;
struct ax_font;

/*
 * Backend
 */

int ax__create_backend(struct ax_state* s, struct ax_backend** out_bac);
void ax__destroy_backend(struct ax_backend* bac);

int ax__event_loop(struct ax_state* s,
                    struct ax_backend* bac);

/*
 * Fonts & text measurement
 */

int ax__create_font(struct ax_state* s, // used for ax__set_error()
                    struct ax_backend* bac,
                    const char* description,
                    struct ax_font** out_font);

void ax__destroy_font(struct ax_font* font);

void ax__measure_text(
    struct ax_font* font,
    const char* text,
    struct ax_text_metrics* out_metrics);
