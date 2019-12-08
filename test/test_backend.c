#include "helpers.h"
#include "../src/backend.h"
#include "../src/utils.h"

struct ax_backend {
};

static struct ax_backend the_backend;

struct ax_font {
    ax_length size;
};

struct ax_backend* ax__create_backend(struct ax_state* s)
{
    (void) s;
    return &the_backend;
}

void ax__destroy_backend(struct ax_backend* bac)
{
    (void) bac;
}

int ax__event_loop(struct ax_state* s)
{
    (void) s;
    NOT_IMPL();
}

struct ax_font* ax__create_font(struct ax_state* s, const char* desc)
{
    (void) s;
    // "size:<N>"
    ASSERT(strncmp(desc, "size:", 5) == 0, "invalid fake font");
    struct ax_font* font = malloc(sizeof(struct ax_font));
    ASSERT(font != NULL, "malloc ax_length for fake font");
    font->size = strtol(desc + 5, NULL, 10);
    return font;
}

void ax__destroy_font(struct ax_font* font)
{
    free(font);
}

void ax__measure_text(
    struct ax_font* font,
    const char* text,
    struct ax_text_metrics* tm)
{
    tm->line_spacing = tm->text_height = font->size;
    tm->width = strlen(text) * font->size;
}
