#include <sys/time.h>
#include <errno.h>
#include "helpers.h"
#include "../src/core.h"
#include "../src/backend.h"
#include "../src/utils.h"
#include "../src/geom/text.h"
#include "backend.h"

struct ax_font {
    ax_length size;
};

int ax__new_backend(struct ax_state* s, struct ax_backend** out_bac)
{
    (void) s;

    struct ax_backend b = {
        .ds = NULL,
        .ds_len = 0,
    };
    pthread_cond_init(&b.sync, NULL);
    pthread_mutex_init(&b.sync_mx, NULL);

    struct ax_backend* bac = malloc(sizeof(struct ax_backend));
    ASSERT(bac != 0, "malloc fake ax_backend");
    *bac = b;
    *out_bac = bac;
    return 0;
}

void ax__destroy_backend(struct ax_backend* bac)
{
    if (bac != NULL) {
        pthread_mutex_destroy(&bac->sync_mx);
        pthread_cond_destroy(&bac->sync);
        free(bac);
    }
}

void ax__set_draws(struct ax_backend* bac,
                   struct ax_draw* draws,
                   size_t len)
{
    pthread_mutex_lock(&bac->sync_mx);
    bac->ds = draws;
    bac->ds_len = len;
    pthread_cond_broadcast(&bac->sync);
    pthread_mutex_unlock(&bac->sync_mx);
}

void ax_test_backend_sync_until(struct ax_backend* bac, size_t desired_len)
{
    bool done = false;
    size_t tries = 0;
    while (!done && tries < 3) {
        pthread_mutex_lock(&bac->sync_mx);
        if (bac->ds_len == desired_len) {
            done = true;
        } else {
            struct timeval now;
            struct timespec targ;
            gettimeofday(&now, NULL);
            targ.tv_sec = now.tv_sec + 1;
            targ.tv_nsec = 0;
            pthread_cond_timedwait(&bac->sync, &bac->sync_mx, &targ);
        }
        pthread_mutex_unlock(&bac->sync_mx);
    }
}

void ax__event_loop(struct ax_backend* bac)
{
    (void) bac;
    NOT_IMPL();
}

int ax__new_font(struct ax_state* s, struct ax_backend* bac,
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
