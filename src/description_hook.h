#pragma once

#include "coordinates.h"

#include <string>

namespace cata
{

/**
 * Fire the `on_description` hook for the game's own detail screen -- the one the
 * look-around cursor opens on its describe key -- which is about to wait for a key.
 *
 * Params handed to Lua: `target` (`creature`, `furniture` or `terrain`, whichever
 * of the three the screen is showing), `text` (that thing's full description, empty
 * when there is nothing of that kind to describe) and `signage` (what is written on
 * the square, which the screen appends whatever the target is).
 *
 * This is the layer's first wall of prose, and the screen offers no way through it:
 * it prints the whole description at once, scrolls nowhere, and accepts only its
 * three keys and the way out. So the text is handed over whole and the Lua side
 * breaks it up -- the alternative would be one utterance several paragraphs long,
 * which is F5 with no remedy.
 *
 * The three "nothing of that kind here" cases are handed over as an empty text
 * rather than as the screen's own sentences, because the screen has three of them
 * and they say the same thing; the wording is the layer's, in `describe.lua`.
 *
 * Fired once per input round, so pressing one of the three keys fires it again with
 * another target -- which is why the Lua side keeps the previous one and says only
 * what changed. Not fired for the input layer's sentinels.
 */
void fire_on_description( const tripoint_bub_ms &p, const std::string &target,
                          const std::string &action );

} // namespace cata
