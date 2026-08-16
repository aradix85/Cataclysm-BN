#pragma once

#include <cstddef>
#include <string>
#include <vector>

class input_context;

namespace cata
{

/**
 * Fire the `on_keybindings` hook for the keybindings screen that is about to
 * wait for a key.
 *
 * That screen is the game's own answer to "which keys work here", reachable
 * from 81 input contexts including the main loop, the inventory, the overmap
 * and trade -- and it is where a key is rebound, so speaking it is also what
 * lets a player set their own keys by ear. It is not a `uilist`, so `on_uilist`
 * never reaches it and it needs a firing point of its own.
 *
 * Params handed to Lua are shaped exactly like `on_uilist`'s -- `category`,
 * `title`, `count`, `cursor`, `entry` with `text` and `column` -- so that the
 * same reading model answers both without a second wording. `category` is the
 * context whose keys are listed rather than this screen's own, which is what
 * makes returning from it to the menu underneath read as arriving somewhere new.
 *
 * The screen has no selection: it scrolls a window over the list and acts on a
 * hotkey letter. The line the scroll offset points at is handed over as the
 * entry, so a keypress that scrolls is answered by the line it scrolled to. The
 * consequence is that the tail of a list shorter than the window cannot be
 * reached at all, because the screen refuses to scroll past what fits; the
 * filter field is the way to the rest.
 *
 * Fired once per input round, so every keypress the screen ignores arrives here
 * too and the handler must work out what changed.
 *
 * Takes the context rather than the screen's fields, because the names and the
 * bound keys are its to answer and it is constructible in a test, which is what
 * keeps the mapping from the scroll offset back to a row under test.
 */
void fire_on_keybindings( const input_context &ctxt, const std::string &category,
                          const std::vector<std::string> &visible,
                          std::size_t scroll_offset );

} // namespace cata
