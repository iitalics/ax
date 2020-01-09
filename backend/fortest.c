#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include "fortest.h"
#include "../test/helpers.h"
#include "../src/core.h"
#include "../src/backend.h"
#include "../src/utils.h"
#include "../src/geom/text.h"

enum {
    SIG_MASK_CLOSE  = 1 << 0,
    SIG_MASK_RESIZE = 1 << 1,
};

struct ax_font {
    ax_length size;
};

int ax__new_backend(struct ax_state* s, struct ax_backend** out_bac)
{
    struct ax_backend* bac = ALLOCATE(&s->init_rgn, struct ax_backend);
    bac->ds = NULL;
    bac->ds_len = 0;
    pthread_mutex_init(&bac->sig_mx, NULL);
    bac->sig.v = 0;
    pthread_cond_init(&bac->sync, NULL);
    pthread_mutex_init(&bac->sync_mx, NULL);
    ax__init_region(&bac->font_rgn);
    *out_bac = bac;
    return 0;
}

void ax__destroy_backend(struct ax_backend* bac)
{
    if (bac != NULL) {
        ax__free_region(&bac->font_rgn);
        pthread_mutex_destroy(&bac->sync_mx);
        pthread_cond_destroy(&bac->sync);
        pthread_mutex_destroy(&bac->sig_mx);
    }
}


bool ax__poll_event(struct ax_backend* bac, struct ax_backend_evt* out_evt)
{
    pthread_mutex_lock(&bac->sig_mx);

#define HANDLE_MASK(_m, _body) do {             \
        if (bac->sig.v & (_m)) {                \
            bac->sig.v &= ~(_m);                \
            _body;                              \
            goto done;                          \
        } } while (0)

    struct ax_backend_evt e = { .ty = AX_BEVT__MAX };

    HANDLE_MASK(SIG_MASK_CLOSE, {
            e.ty = AX_BEVT_CLOSE;
        });

    HANDLE_MASK(SIG_MASK_RESIZE, {
            e.ty = AX_BEVT_RESIZE;
            e.resize_dim = bac->sig.size;
        });

#undef HANDLE_MASK

done:
    pthread_mutex_unlock(&bac->sig_mx);

    if (e.ty < AX_BEVT__MAX) {
        *out_evt = e;
        return true;
    } else {
        return false;
    }
}

void ax__wait_for_frame(struct ax_backend* bac)
{
    (void) bac;
    usleep(5000);
}

void ax_test_backend_sig_close(struct ax_backend* bac)
{
    pthread_mutex_lock(&bac->sig_mx);
    bac->sig.v |= SIG_MASK_CLOSE;
    pthread_mutex_unlock(&bac->sig_mx);
}

void ax_test_backend_sig_resize(struct ax_backend* bac, struct ax_dim size)
{
    pthread_mutex_lock(&bac->sig_mx);
    bac->sig.v |= SIG_MASK_RESIZE;
    bac->sig.size = size;
    pthread_mutex_unlock(&bac->sig_mx);
}

void ax__render(struct ax_backend* bac,
                struct ax_draw* draws,
                size_t len)
{
    pthread_mutex_lock(&bac->sync_mx);
    bac->ds = draws;
    bac->ds_len = len;
    pthread_cond_broadcast(&bac->sync);
    pthread_mutex_unlock(&bac->sync_mx);
}

void ax_test_backend_sync(struct ax_backend* bac)
{
    pthread_mutex_lock(&bac->sync_mx);
    pthread_cond_wait(&bac->sync, &bac->sync_mx);
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

int ax__new_font(struct ax_state* s, struct ax_backend* bac,
                 const char* desc, struct ax_font** out_font)
{
    // "size:<N>"
    if (strncmp(desc, "size:", 5) != 0) {
        ax__set_error(s, "invalid fake font");
        return 1;
    }
    struct ax_font* font = ALLOCATE(&bac->font_rgn, struct ax_font);
    font->size = strtol(desc + 5, NULL, 10);
    *out_font = font;
    return 0;
}

void ax__destroy_font(struct ax_font* font) { (void) font; }

void ax__measure_text(
    struct ax_font* font,
    const char* text,
    struct ax_text_metrics* tm)
{
    tm->line_spacing = tm->text_height = font->size;
    tm->width = strlen(text) * font->size;
}
