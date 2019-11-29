#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "../src/ax.h"
#include "../src/utils.h"
#include "desc_helpers.h"

/*
static ax_color lerp_colors(ax_color c0, ax_color c1, int t, int tmax)
{
    uint8_t rgb0[3], rgb1[3], rgbL[3];
    ax_color_rgb(c0, rgb0);
    ax_color_rgb(c1, rgb1);
    for (size_t i = 0; i < 3; i++) {
        rgbL[i] = ((rgb1[i] - rgb0[i]) * t + rgb0[i] * tmax) / tmax;
    }
    return ax_rgb_color(rgbL);
}
*/

static SDL_Color sdl_color(ax_color c)
{
    uint8_t rgb[3];
    ax_color_rgb(c, rgb);
    return (SDL_Color) { .a = 0xff, .r = rgb[0], .g = rgb[1], .b = rgb[2] };
}


int main(int argc, char** argv)
{
    (void) argc, (void) argv;
    int rv;

    SDL_Window* win = NULL;
    SDL_Renderer* rn = NULL;
    struct ax_state* ax = NULL;
    TTF_Font* a_font = NULL;

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

    a_font = TTF_OpenFont("/usr/share/fonts/TTF/Roboto-Light.ttf", 50);
    TTF_SetFontHinting(a_font, TTF_HINTING_NORMAL);
    if (a_font == NULL) {
        goto ttf_error;
    }

    /*
    struct ax_flex_child_desc children[40];
    for (size_t i = 0; i < LENGTH(children); i++) {
        int fill = lerp_colors(0xffcc11, // #fc1
                               0x8822ff, // #82f
                               i, LENGTH(children));
        children[i] = (struct ax_flex_child_desc)
            FLEX_CHILD(
                0, 1, AX_JUSTIFY_CENTER,
                RECT(fill, 5 + i * 5, 5 + i * 2));
    }

    const struct ax_desc root_desc =
        ACONT(AX_JUSTIFY_EVENLY,
              AX_JUSTIFY_BETWEEN,
              children);
    */

    const struct ax_desc root_desc =
        CONT(
            AX_JUSTIFY_CENTER, AX_JUSTIFY_CENTER,
            FLEX_CHILD(0, 1, 0,
                       TEXT(0x000000, "Hello, world", a_font)));

    ax = ax_new_state();
    ax_set_dimensions(ax, AX_DIM(width, height));
    ax_set_root(ax, &root_desc);

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
        ax_set_dimensions(ax, AX_DIM(win_w, win_h));

        const struct ax_drawbuf* draw = ax_draw(ax);
        for (size_t i = 0; i < draw->len; i++) {
            struct ax_draw d = draw->data[i];
            switch (d.ty) {

            case AX_DRAW_RECT: {
                SDL_Color color = sdl_color(d.r.fill);
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
                SDL_Color fg = sdl_color(d.t.color);
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

sdl_error:
    printf("ERROR: %s\n", SDL_GetError());
    rv = 1;
    goto cleanup;

ttf_error:
    printf("ERROR: %s\n", TTF_GetError());
    rv = 1;
    goto cleanup;

cleanup:
    if (a_font != NULL) {
        TTF_CloseFont(a_font);
    }
    ax_destroy_state(ax);
    SDL_DestroyWindow(win);
    if (TTF_WasInit()) {
        TTF_Quit();
    }
    SDL_Quit();
    return rv;
}