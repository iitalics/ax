#pragma once
#include "base.h"

struct ax_backend;
struct ax_font;

struct ax_state;
struct ax_draw;
struct ax_text_metrics;

/*
 * Backend events
 */

enum ax_backend_evt_type {
    AX_BEVT_CLOSE = 0,
    AX_BEVT_RESIZE,
    AX_BEVT__MAX
};

struct ax_backend_evt {
    enum ax_backend_evt_type ty;
    union {
        struct ax_dim resize_dim;
    };
};

/*
 * Backend
 */

int ax__new_backend(struct ax_state* s, struct ax_backend** out_bac);
void ax__destroy_backend(struct ax_backend* bac);

bool ax__poll_event(struct ax_backend* bac, struct ax_backend_evt* out_evt);

void ax__wait_for_frame(struct ax_backend* bac);

void ax__render(struct ax_backend* bac,
                struct ax_draw* draws,
                size_t len);

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
