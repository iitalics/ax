#pragma once
#include "base.h"

/*
 * Setup and teardown
 */

extern struct ax_state* ax_new_state();
extern void ax_destroy_state(struct ax_state* s);

/*
 * Basic operations
 */

extern const char* ax_get_error(struct ax_state* s);

/*
 * S-exp interface
 */

extern void ax_write_start(struct ax_state* s);
extern int ax_write_chunk(struct ax_state* s, const char* input);
extern int ax_write_end(struct ax_state* s);

static inline int ax_write(struct ax_state* s, const char* input)
{
    ax_write_start(s);
    int r;
    if ((r = ax_write_chunk(s, input)) != 0) {
        return r;
    }
    return ax_write_end(s);
}

/*
 * Drawing
 */

extern const struct ax_draw_buf* ax_draw(struct ax_state* s);
