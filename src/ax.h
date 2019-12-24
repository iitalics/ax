#pragma once
#include <stdlib.h>

/*
 * Setup and teardown
 */

struct ax_state* ax_new_state(void);
void ax_destroy_state(struct ax_state* s);

/*
 * Basic operations
 */

const char* ax_get_error(struct ax_state* s);

// TODO: replace with eg. "wait_for_event" or "poll_event"
int ax_wait_for_close(struct ax_state* s);

/*
 * S-exp interface
 */

void ax_write_start(struct ax_state* s);
int ax_write_chunk(struct ax_state* s, const char* input, size_t len);
int ax_write_end(struct ax_state* s);
int ax_write(struct ax_state* s, const char* input);
