#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "../src/ax.h"
#include "../src/utils.h"
#include "../src/text.h"

ax_color ax__lerp_colors(ax_color c0, ax_color c1, int t, int tmax)
{
    uint8_t rgb0[3], rgb1[3], rgbL[3];
    ax_color_rgb(c0, rgb0);
    ax_color_rgb(c1, rgb1);
    for (size_t i = 0; i < 3; i++) {
        rgbL[i] = ((rgb1[i] - rgb0[i]) * t + rgb0[i] * tmax) / tmax;
    }
    return ax_rgb_color(rgbL);
}

SDL_Color ax__sdl_color(ax_color c)
{
    uint8_t rgb[3];
    ax_color_rgb(c, rgb);
    return (SDL_Color) { .a = 0xff, .r = rgb[0], .g = rgb[1], .b = rgb[2] };
}


void ax__measure_text(
    void* font_voidptr,
    const char* text,
    struct ax_text_metrics* tm)
{
    TTF_Font* font = font_voidptr;

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


void* ax__create_font(const char* name)
{
    // "size:<N>,path:<PATH>"
    char* s = (char*) name;
    ASSERT(strncmp(s, "size:", 5) == 0, "invalid font");
    long size = strtol(s + 5, &s, 10);
    ASSERT(strncmp(s, ",path:", 6) == 0, "invalid font");
    char* path = s + 6;
    return TTF_OpenFont(path, size);
}

void ax__destroy_font(void* font)
{
    TTF_CloseFont(font);
}



static int build_example(struct ax_state* ax, size_t n)
{
    char buf[1024];
    char* s = buf;
    s += sprintf(s, "(set-root (container (children");
    for (size_t i = 0; i < n; i++) {
        s += sprintf(s,
                     "(rect (fill \"%06x\")"
                     "      (size %zu %zu)"
                     "      (shrink %d))",
                     ax__lerp_colors(0xffcc11, 0x8822ff, i, n),
                     100 + i * 5, 100 + i * 30,
                     i == 0 ? 0 : 1);
    }
    s += sprintf(s, ")"
                 "  (main-justify evenly)"
                 "  (cross-justify between)"
                 "  single-line))");
    printf("(%zu bytes)\n", s - buf);
    return ax_read(ax, buf);
}


int main(int argc, char** argv)
{
    (void) argc, (void) argv;
    int rv;

    SDL_Window* win = NULL;
    SDL_Renderer* rn = NULL;
    struct ax_state* ax = NULL;

    const int width = 800;
    const int height = 400;

    SDL_Init(SDL_INIT_VIDEO);
    if (SDL_CreateWindowAndRenderer(
            width, height,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN,
            &win, &rn) != 0) {
        goto sdl_error;
    }

    if (TTF_Init() != 0) {
        goto ttf_error;
    }

    ax = ax_new_state();
    build_example(ax, 5);

    for (;;) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {

            case SDL_QUIT:
                printf("quit event\n");
                goto exit;

            case SDL_KEYDOWN:
                if (ev.key.keysym.sym == SDLK_q) {
                    goto exit;
                }
                break;

            default:
                break;
            }
        }

        SDL_SetRenderDrawColor(rn, 0xff, 0xff, 0xff, 0xff);
        SDL_RenderClear(rn);

        int win_w, win_h;
        SDL_GetWindowSize(win, &win_w, &win_h);

        char set_dim_msg[128];
        sprintf(set_dim_msg, "(set-dim %d %d)", win_w, win_h);
        if (ax_read(ax, set_dim_msg) != 0) { goto ax_error; }

        const struct ax_drawbuf* draw = ax_draw(ax);
        for (size_t i = 0; i < draw->len; i++) {
            struct ax_draw d = draw->data[i];
            switch (d.ty) {

            case AX_DRAW_RECT: {
                SDL_Color color = ax__sdl_color(d.r.fill);
                SDL_SetRenderDrawColor(rn, color.r, color.g, color.b, color.a);
                SDL_Rect r;
                r.x = d.r.bounds.o.x;
                r.y = d.r.bounds.o.y;
                r.w = d.r.bounds.s.w;
                r.h = d.r.bounds.s.h;
                SDL_RenderFillRect(rn, &r);
                break;
            }

            case AX_DRAW_TEXT: {
                SDL_Color fg = ax__sdl_color(d.t.color);
                SDL_Surface* sf = TTF_RenderUTF8_Blended(d.t.font, d.t.text, fg);
                if (sf == NULL) {
                    goto ttf_error;
                }
                SDL_Texture* tx = SDL_CreateTextureFromSurface(rn, sf);
                if (tx == NULL) {
                    SDL_FreeSurface(sf);
                    goto sdl_error;
                }
                SDL_Rect r;
                r.x = d.t.pos.x;
                r.y = d.t.pos.y;
                r.w = sf->w;
                r.h = sf->h;
                SDL_RenderCopy(rn, tx, NULL, &r);
                SDL_DestroyTexture(tx);
                SDL_FreeSurface(sf);
                break;
            }

            default: NO_SUCH_TAG("ax_draw_type");
            }
        }

        SDL_RenderPresent(rn);
        SDL_Delay(16);
    }

exit:
    rv = 0;
    goto cleanup;

ax_error:
    printf("ERROR: %s\n", ax_get_error(ax));
    rv = 1;
    goto cleanup;

sdl_error:
    printf("ERROR: %s\n", SDL_GetError());
    rv = 1;
    goto cleanup;

ttf_error:
    printf("ERROR: %s\n", TTF_GetError());
    rv = 1;
    goto cleanup;

cleanup:
    ax_destroy_state(ax);
    SDL_DestroyWindow(win);
    if (TTF_WasInit()) {
        TTF_Quit();
    }
    SDL_Quit();
    return rv;
}
