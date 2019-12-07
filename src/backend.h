#pragma once
#include "geom/text.h" // TODO: don't depend on text.h

void* ax__create_font(const char* name);
void ax__destroy_font(void* font);

void ax__measure_text(
    void* font,
    const char* text,
    struct ax_text_metrics* out_metrics);
