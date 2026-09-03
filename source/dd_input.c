/* dd_input.c -- controller -> synthetic touch for Data Defense.
 *
 * WHY THE KILLER BEAN VERSION HAD TO GO
 * -------------------------------------
 * The tree this was retargeted from carried dd_input.c, which maps controller
 * buttons to synthetic touches at HARDCODED NORMALISED SCREEN COORDINATES --
 * the exact spots where Killer Bean Unleashed draws its own HUD sprites:
 *
 *      jump   A            (0.8172, 0.9347)
 *      gun    B            (0.8961, 0.8194)
 *      green  D-pad right  (0.7320, 0.9375)
 *      prev   L            (0.0898, 0.0833)
 *      next   R            (0.1445, 0.0833)
 *
 * Those numbers describe one specific game's user interface. Data Defense is a
 * landscape tower defense with no jump button, no gun and no weapon carousel;
 * at those coordinates it has map tiles and tower slots. Inheriting the file
 * would not have failed to build or crashed -- it would have made A and B
 * place or sell towers at fixed points on the board, which is the kind of bug
 * that gets diagnosed as "the game is broken" rather than "the input mapping
 * belongs to a different game".
 *
 * It was also not dead code: android_native_unity.c calls dd_input_poll()
 * unconditionally in its event pump, so the mapping would have been live from
 * the first frame.
 *
 * WHAT THIS DOES INSTEAD
 * ----------------------
 * Nothing, deliberately, until real mappings are derived. Data Defense is
 * cursor-driven: the touchscreen works directly in handheld, and in docked
 * mode nx_pointer.c drives an on-screen cursor from the stick, gyro or a
 * USB mouse, with a tap on ZL/ZR. That covers the whole game -- placing,
 * selling and upgrading towers are all taps -- so there is no button that
 * MUST be mapped for the game to be playable.
 *
 * The function is kept, with its original name and signature, so
 * android_native_unity.c compiles and links unchanged. Returning 0 is a
 * correct, complete implementation of "no buttons are mapped yet".
 *
 * HOW TO ADD MAPPINGS LATER
 * -------------------------
 * Only add a button once you have watched the game and know where the control
 * actually sits. The candidates worth having, in rough order of value:
 *
 *   - a "start wave" / speed-up button, if the game has a persistent one
 *   - B as Android Back, for closing panels (see nativeInjectEvent and the
 *     key path in unity_input.c -- Back is a KEYCODE, not a touch, so it does
 *     NOT belong in this file)
 *   - shoulder buttons for tower-type selection, if the palette is at fixed
 *     screen positions
 *
 * Take the coordinates from a screenshot at a known resolution and store them
 * normalised, exactly as the Killer Bean table did. The mechanism was sound;
 * only its data was wrong.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "dd_input.h"

int dd_input_poll(NxpEvent *ev, int room, int screen_w, int screen_h)
{
    (void)ev; (void)room; (void)screen_w; (void)screen_h;
    return 0;
}
