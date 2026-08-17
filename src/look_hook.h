#pragma once

#include "coordinates.h"
#include "enums.h"

#include <string>

namespace cata
{

/**
 * How well the character can make out a square, as a token rather than a
 * sentence: `clear`, `lit`, `dark`, `blur`, `blur_dark`, `hidden`.
 *
 * The game's own words for these live in `game::print_visibility_info`, which
 * writes them into a window and returns nothing, so they cannot be read back.
 * The wording is therefore the layer's, in `look.lua`, and this hands over which
 * of the six cases it is.
 */
std::string sight_of( visibility_type visibility );

/**
 * Fire the `on_look_around` hook for the look-around screen that is about to wait
 * for a key.
 *
 * Params handed to Lua: `cursor` (the square under the cursor), `sight` (the token
 * above), `peeking` (whether this is the screen the peek key opens rather than the
 * look key) and `action` (what the previous round answered, empty at the firing
 * that opens the screen).
 *
 * The square itself is handed over rather than described, because everything the
 * screen prints about one is already reachable from script: the terrain and
 * furniture through the map, the creature through `gapi`, the sound and the region
 * name through `perception`. What script cannot work out is how much of the square
 * the character can actually make out, which is what `sight` is for -- and that is
 * F3, the failure where a mechanic that works is invisible to the player.
 *
 * Silent unless the screen is showing its info panel. `game::look_around` is also
 * the cursor for marking out a zone and for dragging one, and in those shapes the
 * caller draws its own window, there is no panel and, while a zone is being
 * dragged, no single square under the cursor to answer about.
 *
 * Fired once per input round rather than once per screen, so moving the cursor
 * fires again -- which is why the Lua side keeps the previous square and says only
 * what changed. Not fired for the input layer's sentinels: this loop takes a
 * timeout for the pixel minimap and for mouse edge scrolling, so it completes
 * rounds without a key being pressed at all.
 */
void fire_on_look_around( const tripoint_bub_ms &cursor, bool show_window, bool peeking,
                          const std::string &action );

} // namespace cata
