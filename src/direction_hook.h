#pragma once

#include <string>

namespace cata
{

/**
 * Fire the `on_direction_prompt` hook for the question the game asks when a verb
 * needs a square to act on: "Close where?", "Smash where?", "Jump across where?".
 *
 * Params handed to Lua: `text` -- the question in the game's own words, without the
 * "(Direction button)" the popup appends -- `vertical`, whether up and down are
 * accepted as well as the eight compass keys, and `action`, what the previous round
 * of the loop answered.
 *
 * This prompt takes the keyboard as completely as a blocking popup does and is not
 * one: `choose_direction` in `src/action.cpp` puts up a `static_popup` and runs an
 * `input_context` of its own, so `on_query_popup` never fires for it and no key
 * registered anywhere else reaches it. Roughly seventy verbs arrive here through
 * `choose_direction`, `choose_adjacent` and `choose_adjacent_highlight`, so this one
 * firing is what the whole family speaks by.
 *
 * An empty `action` is the prompt opening; a later firing means the previous key did
 * not answer it, since any key that does ends the loop.
 */
void fire_on_direction_prompt( const std::string &message, bool allow_vertical,
                               const std::string &action );

} // namespace cata
