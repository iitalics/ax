#pragma once
#include "base.h"

/*
 * Setup and teardown
 */

struct ax_state* ax_new_state();
void ax_destroy_state(struct ax_state* s);

/*
 * Basic operations
 */

int ax_event_loop(struct ax_state* s);

const char* ax_get_error(struct ax_state* s);

/*
 * S-exp interface
 */

void ax_write_start(struct ax_state* s);
int ax_write_chunk(struct ax_state* s, const char* input);
int ax_write_end(struct ax_state* s);
int ax_write(struct ax_state* s, const char* input);

/*
 * Drawing
 */

const struct ax_draw_buf* ax_draw(struct ax_state* s);
