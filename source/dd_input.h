/* dd_input.h -- controller -> synthetic touch for Data Defense.
 *
 * Replaces the Killer Bean tree's dd_input.c. See dd_input.c for why that file
 * could not be inherited.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */
#ifndef __DD_INPUT_H__
#define __DD_INPUT_H__

#include "nx_pointer.h"

/* Append synthetic touch events for held/pressed buttons. Returns the number
 * written, never more than `room`. Currently always 0 -- see dd_input.c. */
int dd_input_poll(NxpEvent *ev, int room, int screen_w, int screen_h);

#endif
