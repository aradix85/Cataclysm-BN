#pragma once

#include <string>
#include <vector>

class inventory_selector;

namespace cata
{

/**
 * One row of the inventory screen, as the screen itself draws it.
 *
 * Category headings are rows too, and they are not here: they are not items,
 * they cannot be selected, and counting them would make the position spoken
 * with an item disagree with the number of items there are.
 */
struct inventory_line {
    /** What the screen draws for the row: the item's name, with its stack count where it has one. */
    std::string text;
    /** The heading the row sits under -- "WEAPONS", "FOOD". */
    std::string category;
    /**
     * Where the item is: "character", "map", "vehicle" or "container".
     *
     * The screens that offer nearby items put them in a column of their own,
     * and the columns are told apart by where they are drawn -- which is
     * nothing at all without sight. The game names the place only when it is
     * another square, by putting a direction into the heading, so an item lying
     * at the player's own feet reads exactly like one in their pack. This is
     * that difference, taken from the item rather than from the column, so it
     * is right per row even where a column holds both.
     */
    std::string where;
    /** Why the row cannot be chosen, in the screen's own words, or empty when it can. */
    std::string denial;
    /** How many of the stack are marked, on the screens that mark: drop, pick up, use. */
    int marked = 0;
    /** Whether the row can be chosen at all. */
    bool enabled = false;
    /** Whether the cursor is on this row. */
    bool selected = false;
};

/**
 * Fire the `on_inventory` hook for an inventory screen about to wait for a key.
 *
 * Params handed to Lua: `title`, `count` (how many item rows the current filter
 * leaves), `cursor` (the 1-based position of the selection among those) and
 * `entry` (the selected row as `{ text, category, where, denial, enabled,
 * marked }`).
 *
 * `cursor` and `entry` are left unset rather than zeroed when nothing is
 * selected -- an empty screen, or a filter that matched nothing -- so a handler
 * cannot read a selection out of a screen that has none.
 *
 * Fired once per input round rather than once per screen, so moving the cursor
 * fires it again with a different `cursor`, and the Lua side says only what
 * changed. Only the selected row is handed over: a full inventory runs to
 * hundreds of rows and copying all of them on every keypress would cost more
 * than drawing the screen does.
 *
 * The overload taking the selector is the one the screen calls, and it is the
 * only part of this that a test cannot reach: an `inventory_selector` needs a
 * player, a map and a curses window. It reads the rows and hands them to the
 * overload below, which is where every decision about what the player hears is
 * made, and which a test drives with rows of its own.
 *
 * No re-entry guard: an inventory screen is not what the game raises to report
 * an error, so a failing handler cannot arrive back here the way it can through
 * a popup.
 */
void fire_on_inventory( inventory_selector &selector );
void fire_on_inventory( const std::string &title, const std::vector<inventory_line> &lines );

} // namespace cata
