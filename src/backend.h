#pragma once
#include "ax.h"
#include "geom/text.h" // TODO: don't depend on text.h

struct ax_backend;
struct ax_font;

/*
 * Backend
 */

struct ax_backend* ax__create_backend(struct ax_state* s);
void ax__destroy_backend(struct ax_backend* bac);

int ax__event_loop(struct ax_state* s);

/*
 * Fonts & text measurement
 */

struct ax_font* ax__create_font(struct ax_state* s,
                                const char* description);

void ax__destroy_font(struct ax_font* font);

void ax__measure_text(
    struct ax_font* font,
    const char* text,
    struct ax_text_metrics* out_metrics);
