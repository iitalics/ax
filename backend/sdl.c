#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "../src/core.h"
#include "../src/geom/text.h"
#include "../src/draw.h"
#include "../src/backend.h"
#include "../src/utils.h"

struct ax_backend {
    SDL_Window* window;
    SDL_Renderer* render;
};

static void free_backend(struct ax_backend* b)
{
    if (b->render != NULL) {
        SDL_DestroyRenderer(b->render);
    }
    if (b->window != NULL) {
        SDL_DestroyWindow(b->window);
    }
    TTF_Quit();
    SDL_Quit();
}

int ax__new_backend(struct ax_state* ax, struct ax_backend** out_bac)
{
    struct ax_backend b = { .window = NULL, .render = NULL };

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        goto sdl_err;
    }
    if (TTF_Init() != 0) {
        goto ttf_err;
    }
    if (SDL_CreateWindowAndRenderer(
            (int) ax->config.win_size.w,
            (int) ax->config.win_size.h,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN,
            &b.window, &b.render) != 0) {
        goto sdl_err;
    }
    if (SDL_SetRenderDrawBlendMode(b.render, SDL_BLENDMODE_BLEND) != 0) {
        goto sdl_err;
    }

    struct ax_backend* bac = malloc(sizeof(struct ax_backend));
    ASSERT(bac != NULL, "malloc ax_backend (SDL)");
    *bac = b;
    *out_bac = bac;
    return 0;

ttf_err:
    ax__set_error(ax, TTF_GetError());
    goto err;
sdl_err:
    ax__set_error(ax, SDL_GetError());
err:
    free_backend(&b);
    return 1;
}

void ax__destroy_backend(struct ax_backend* bac)
{
    if (bac != NULL) {
        free_backend(bac);
        free(bac);
    }
}

static SDL_Color ax_color_to_sdl(ax_color c)
{
    uint8_t rgb[3];
    if (ax_color_to_rgb(c, rgb)) {
        return (SDL_Color) {
            .a = 0xff,
            .r = rgb[0],
            .g = rgb[1],
            .b = rgb[2],
        };
    } else {
        return (SDL_Color) { .a = 0 };
    }
}

static int draw(struct ax_state* ax)
{
    SDL_SetRenderDrawColor(ax->backend->render, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderClear(ax->backend->render);

    const struct ax_draw_buf* draw = ax->draw_buf;
    for (size_t i = 0; i < draw->len; i++) {
        struct ax_draw d = draw->data[i];
        switch (d.ty) {

        case AX_DRAW_RECT: {
            SDL_Color color = ax_color_to_sdl(d.r.fill);
            SDL_SetRenderDrawColor(ax->backend->render, color.r, color.g, color.b, color.a);
            SDL_Rect r;
            r.x = d.r.bounds.o.x;
            r.y = d.r.bounds.o.y;
            r.w = d.r.bounds.s.w;
            r.h = d.r.bounds.s.h;
            SDL_RenderFillRect(ax->backend->render, &r);
            break;
        }

        case AX_DRAW_TEXT: {
            SDL_Color fg = ax_color_to_sdl(d.t.color);
            SDL_Surface* sf = TTF_RenderUTF8_Blended((void*) d.t.font, d.t.text, fg);
            if (sf == NULL) {
                goto ttf_err;
            }
            SDL_Texture* tx = SDL_CreateTextureFromSurface(ax->backend->render, sf);
            if (tx == NULL) {
                SDL_FreeSurface(sf);
                goto sdl_err;
            }
            SDL_Rect r;
            r.x = d.t.pos.x;
            r.y = d.t.pos.y;
            r.w = sf->w;
            r.h = sf->h;
            SDL_RenderCopy(ax->backend->render, tx, NULL, &r);
            SDL_DestroyTexture(tx);
            SDL_FreeSurface(sf);
            break;
        }

        default: NO_SUCH_TAG("ax_draw_type");
        }
    }

    SDL_RenderPresent(ax->backend->render);
    return 0;

ttf_err:
    ax__set_error(ax, TTF_GetError());
    return 1;
sdl_err:
    ax__set_error(ax, SDL_GetError());
    return 1;
}

int ax__event_loop(struct ax_state* ax, struct ax_backend* bac)
{
    (void) bac;
    for (;;) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_QUIT:
                return 0;
            case SDL_KEYDOWN:
                switch (ev.key.keysym.sym) {
                case SDLK_q:
                    return 0;
                default: break;
                }
                break;
            default: break;
            }
        }

        int win_w, win_h;
        SDL_GetWindowSize(ax->backend->window, &win_w, &win_h);
        ax__set_dim(ax, AX_DIM(win_w, win_h));

        int rv;
        if ((rv = draw(ax)) != 0) {
            return rv;
        }
        SDL_Delay(16);
    }
}

int ax__new_font(struct ax_state* ax,
                 struct ax_backend* bac,
                 const char* description,
                 struct ax_font** out_font)
{
    (void) bac;

    // "size:<N>,path:<PATH>"
    char* s = (char*) description;
    if (strncmp(s, "size:", 5) != 0) {
        goto err;
    }
    long size = strtol(s + 5, &s, 10);
    if (strncmp(s, ",path:", 6) != 0) {
        goto err;
    }
    char* path = s + 6;
    void* f = TTF_OpenFont(path, size);
    if (f == NULL) {
        ax__set_error(ax, TTF_GetError());
        return 1;
    }
    *out_font = f;
    return 0;
err:
    ax__set_error(ax, "invalid font description");
    return 1;
}

void ax__destroy_font(struct ax_font* font)
{
    TTF_CloseFont((void*) font);
}

void ax__measure_text(
    struct ax_font* font_,
    const char* text,
    struct ax_text_metrics* tm)
{
    TTF_Font* font = (void*) font_;
    int w_int;
    if (text == NULL) {
        w_int = 0;
    } else {
        int rv = TTF_SizeUTF8(font, text, &w_int, NULL);
        ASSERT(rv == 0, "TTF_SizeText failed");
    }
    tm->text_height = TTF_FontHeight(font);
    tm->line_spacing = TTF_FontLineSkip(font);
    tm->width = w_int;
}
