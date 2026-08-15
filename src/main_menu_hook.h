#pragma once

#include <string>
#include <vector>

namespace cata
{

/**
 * Fire the `on_main_menu` hook for the opening screen, before it waits for a key.
 *
 * The screen the game opens on is not a `uilist`, so `on_uilist` never reaches
 * it: `main_menu::opening_screen` builds an `input_context` of its own and draws
 * itself through `print_menu`. It is the first screen a player meets and the one
 * they come back to after leaving a world, and without sight it is otherwise
 * silent.
 *
 * It reads as two lists at once. `heading` is the row along the top -- MOTD, New
 * Game, Load, World, Settings, Help, Credits, Quit -- and `entry` is the list
 * drawn under whichever of those is selected. Each arrives as
 * `{ text, key, cursor, count }`, with `cursor` 1-based and `key` the letter
 * that jumps straight to it, empty where the game marks none.
 */
/**
 * `entry` is left unset rather than zeroed where there is nothing under the
 * heading to select: Help and Quit carry no list, MOTD and Credits show a page
 * of text rather than one, and Load carries none until a world exists. A handler
 * can then not read a selection out of a level that has none.
 *
 * The hotkey markup the game draws -- `<N|n>ew Game` -- is resolved to the plain
 * name here, using the game's own function, so what reaches Lua is what a
 * synthesiser can say.
 *
 * Fired once per input round rather than once per screen, so moving the
 * selection fires it again with a different cursor. That is `on_uilist`'s
 * contract as well, and for the same reason: the screen has no event for a
 * selection changing, only a loop that redraws and waits.
 *
 * Takes the two static sub-item lists rather than reading them, because they are
 * private to `main_menu`; the world lists it reads for itself.
 */
void fire_on_main_menu( const std::vector<std::string> &headings, int heading_sel,
                        const std::vector<std::string> &new_game_items,
                        const std::vector<std::string> &settings_items, int entry_sel );

} // namespace cata
