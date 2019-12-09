#include "helpers.h"
#include "../src/core.h"
#include "../src/backend.h"
#include "../src/utils.h"
#include "../src/geom/text.h"

struct ax_backend {};
static struct ax_backend the_backend;

struct ax_font {
    ax_length size;
};

int ax__create_backend(struct ax_state* s, struct ax_backend** out_bac)
{
    (void) s;
    *out_bac = &the_backend;
    return 0;
}

void ax__destroy_backend(struct ax_backend* bac)
{
    (void) bac;
}

int ax__event_loop(struct ax_state* s, struct ax_backend* bac)
{
    (void) s;
    NOT_IMPL();
}

int ax__create_font(struct ax_state* s, struct ax_backend* bac,
                    const char* desc, struct ax_font** out_font)
{
    (void) bac;
    // "size:<N>"
    if (strncmp(desc, "size:", 5) != 0) {
        ax__set_error(s, "invalid fake font");
        return 1;
    }
    struct ax_font* font = malloc(sizeof(struct ax_font));
    ASSERT(font != NULL, "malloc ax_length for fake font");
    font->size = strtol(desc + 5, NULL, 10);
    *out_font = font;
    return 0;
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
