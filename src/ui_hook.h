#pragma once

#include <string>
#include <vector>

struct uilist_entry;

namespace cata
{

/**
 * Fire the `on_uilist` hook for a menu that is about to wait for a key.
 *
 * Params handed to Lua: `category` (the menu's input context, "UILIST" unless
 * the menu overrides it), `title`, `text` (the header drawn above the list),
 * `count` (how many entries the current filter leaves visible), `cursor` (the
 * 1-based position of the selection among those) and `entry` (the selected
 * entry as `{ text, desc, column, enabled }`).
 *
 * `cursor` and `entry` are left unset rather than zeroed when the filter leaves
 * nothing to select, so a handler cannot read a selection out of a menu that
 * has none.
 *
 * Fired once per input round rather than once per menu, so moving the selection
 * fires it again with a different `cursor`. Only the selected entry is handed
 * over: a menu can hold hundreds, and copying all of them into a table on every
 * keypress would cost more than drawing the menu does.
 *
 * Takes the menu's fields rather than the menu itself, because `uilist` refuses
 * to be constructed in test mode while a `uilist_entry` does not. That keeps the
 * mapping from the filtered list back to the entries under test instead of only
 * under the game.
 *
 * No re-entry guard: a menu is not what the game raises to report an error, so a
 * failing handler cannot arrive back here the way it can through a popup.
 */
void fire_on_uilist( const std::string &category, const std::string &title,
                     const std::string &text, const std::vector<uilist_entry> &entries,
                     const std::vector<int> &filtered, int cursor );

} // namespace cata
