#pragma once
#include <stdlib.h>
#include <stdbool.h>

/*
 * Setup and teardown
 */

struct ax_state* ax_new_state(void);
void ax_destroy_state(struct ax_state* s);

/*
 * Basic operations
 */

const char* ax_get_error(struct ax_state* s);

// the returned fd becomes "ready to read" when there are available events. however, you
// shouldn't read from it directly; rather use one of the 'read' functions below.
int ax_poll_event_fd(struct ax_state* s);

// returns 'true' if one of the 'read' functions below would not block.
bool ax_poll_event(struct ax_state* s);

// blocks until the window is closed. currently 'close' is the only kind of event. in the
// future there needs to be a mechanism to read one event, then dispatch on what kind it
// is.
void ax_read_close_event(struct ax_state* s);

/*
 * S-exp interface
 */

void ax_write_start(struct ax_state* s);
int ax_write_chunk(struct ax_state* s, const char* input, size_t len);
int ax_write_string(struct ax_state* s, const char* input);
int ax_write_end(struct ax_state* s);
int ax_write(struct ax_state* s, const char* input);
