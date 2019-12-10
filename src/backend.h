#pragma once
#include "ax.h"

struct ax_backend;
struct ax_font;
struct ax_draw;
struct ax_text_metrics;

/*
 * Backend
 */

int ax__new_backend(struct ax_state* s, struct ax_backend** out_bac);
void ax__destroy_backend(struct ax_backend* bac);

void ax__set_draws(struct ax_backend* bac,
                   struct ax_draw* draws,
                   size_t len);

void ax__event_loop(struct ax_backend* bac);

/*
 * Fonts & text measurement
 */

int ax__new_font(struct ax_state* s, // used for ax__set_error()
                 struct ax_backend* bac,
                 const char* description,
                 struct ax_font** out_font);

void ax__destroy_font(struct ax_font* font);

void ax__measure_text(
    struct ax_font* font,
    const char* text,
    struct ax_text_metrics* out_metrics);
