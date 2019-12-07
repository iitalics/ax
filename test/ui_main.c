#include <stdio.h>
#include "../src/ax.h"

/* ax_color ax__lerp_colors(ax_color c0, ax_color c1, int t, int tmax) */
/* { */
/*     uint8_t rgb0[3], rgb1[3], rgbL[3]; */
/*     ax_color_to_rgb(c0, rgb0); */
/*     ax_color_to_rgb(c1, rgb1); */
/*     for (size_t i = 0; i < 3; i++) { */
/*         rgbL[i] = ((rgb1[i] - rgb0[i]) * t + rgb0[i] * tmax) / tmax; */
/*     } */
/*     return ax_color_from_rgb(rgbL); */
/* } */

/*
 * Demo
 */

#define ROBOTO "/usr/share/fonts/TTF/Roboto-Light.ttf"

static int build_example(struct ax_state* ax, size_t n)
{
    char buf[2024];
    char* s = buf;
    s += sprintf(s, "(set-root (container (children");
    for (size_t i = 0; i < n; i++) {
        s += sprintf(s,
                     "(text \"Hewwo?!\""
                     "      (font \"size:%zu,path:" ROBOTO "\"))",
                     10 + i * 5);
    }
    s += sprintf(s,
                 ")"
                 //" (background \"ccddff\")"
                 " (main-justify evenly)"
                 " (cross-justify between)"
                 " multi-line))");
    printf("(%zu bytes)\n", s - buf);
    printf("%s\n", buf);
    return ax_write(ax, buf);
}

/*
 * main
 */

int main(int argc, char** argv)
{
    (void) argc, (void) argv;
    int rv;

    struct ax_state* ax = ax_new_state();

    if ((rv = build_example(ax, 10)) != 0) {
        goto err;
    }

    if ((rv = ax_event_loop(ax)) != 0) {
        goto err;
    }

    goto cleanup;

err:
    fprintf(stderr, "ERROR: %s\n", ax_get_error(ax));

cleanup:
    ax_destroy_state(ax);
    return rv;
}
