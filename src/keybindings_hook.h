#pragma once

#include <cstddef>
#include <string>
#include <vector>

class input_context;

namespace cata
{

/**
 * Where a scrolling list is being read: which row is selected, and which row
 * the window starts at.
 */
struct list_position {
    std::size_t selected = 0;
    std::size_t offset = 0;
};

/**
 * Move the selection by `delta` rows and drag the window after it.
 *
 * The keybindings screen was built to scroll a window and act on a hotkey
 * letter, with nothing selected at all. That is unusable without sight: a list
 * that already fits refuses to scroll, so every arrow key answers with silence
 * and only the first row can ever be reached. A selection is what makes the
 * list walkable, and the window follows it instead of replacing it.
 *
 * `wrap` belongs to a single step, which is what every other list in the game
 * does at its ends; a page move clamps instead, because a page is a distance
 * rather than a step and landing at the far end of the list is not what was
 * asked for.
 *
 * Pure arithmetic, kept out of the screen so it is asserted without drawing
 * anything: the screen itself blocks on input and cannot be driven by a test.
 */
list_position move_selection( list_position at, int delta, std::size_t size,
                              std::size_t height, bool wrap );

/**
 * Put the selection back inside a list that changed under it, which is what
 * typing into the filter does on every keypress.
 */
list_position clamp_selection( list_position at, std::size_t size, std::size_t height );

/**
 * The keybindings screen as it stands at one firing.
 *
 * `hotkeys` are the letters the screen draws down the left of the window, in
 * view order, so the letter belonging to the selected row is found by taking
 * it minus `offset`. They only act while `picking` -- outside that mode the
 * screen draws no letters and a keypress goes into the filter instead.
 *
 * `adds_local`, `adds_global` and `removes` are the screen's own three keys for
 * starting those modes, taken from where it draws its legend rather than
 * repeated here, so a change upstream cannot leave the layer saying the wrong
 * key.
 */
struct keybindings_screen {
    std::size_t selected = 0;
    std::size_t offset = 0;
    std::string hotkeys;
    char adds_local = '\0';
    char adds_global = '\0';
    char removes = '\0';
    bool picking = false;
};

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
 * Params handed to Lua are shaped like `on_uilist`'s -- `category`, `title`,
 * `count`, `cursor`, `entry` with `text` and `column` -- so that the same
 * reading model answers both without a second wording. `category` is the
 * context whose keys are listed rather than this screen's own, which is what
 * makes returning from it to the menu underneath read as arriving somewhere new.
 *
 * Three params are this screen's alone: `picking`, whether letters currently
 * act on rows; `entry.letter`, the one that picks the selected row, set only
 * while they do; and `keys`, the three that start those modes. Without them the
 * screen can be read but not used, since choosing a row means pressing a letter
 * that is drawn and never otherwise said.
 *
 * The rest of rebinding needs nothing here: every question it asks after a row
 * is picked is a `query_popup`, which the layer already speaks.
 *
 * Fired once per input round, so every keypress the screen ignores arrives here
 * too and the handler must work out what changed.
 *
 * Takes the context rather than the screen's fields, because the names and the
 * bound keys are its to answer and it is constructible in a test, which is what
 * keeps the mapping from a row back to its name under test.
 */
void fire_on_keybindings( const input_context &ctxt, const std::string &category,
                          const std::vector<std::string> &visible,
                          const keybindings_screen &at );

} // namespace cata
