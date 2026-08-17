#pragma once

#include <array>
#include <string>
#include <vector>

#include "advanced_inv_area.h"

class advanced_inventory_pane;

namespace cata
{

/**
 * One of the two places the screen is working between.
 *
 * The screen is built out of two panes and the whole of it is that pair: items
 * leave the pane the cursor is in and arrive in the other one. Which is which
 * is drawn as a colour and a position, so without sight the pair has to be
 * said, and neither half of it means anything alone.
 */
struct advanced_inventory_place {
    /** What the pane is aimed at, in the screen's own words: "Inventory", "South West", "Worn Items". */
    std::string area;
    /**
     * The vehicle whose cargo the pane is showing, or empty when it is showing
     * the ground.
     *
     * A square with a vehicle on it holds two separate places, and the key that
     * swaps between them changes nothing else the player can hear. The screen
     * answers by drawing the vehicle's name in place of the direction; both are
     * handed over, because the direction is the half that says where to walk.
     */
    std::string vehicle;
};

/**
 * One row of a pane, as the screen itself draws it.
 *
 * Category headers are rows too, and they are not here, for the reason they are
 * left out of the plain inventory: they cannot be selected, and counting them
 * would put every position spoken out by however many sit above the cursor.
 */
struct advanced_inventory_line {
    /** The item's name, with the size of the stack in front of it when there is more than one. */
    std::string text;
    /**
     * The heading the row sits under, and empty when the screen draws none.
     *
     * This screen only groups when it is sorted by category; sorted any other
     * way it draws a flat list, and a heading said for a row that has none
     * would change with nearly every keypress.
     */
    std::string category;
    /**
     * Which square the item is on, and empty unless the pane is showing all of
     * them at once.
     *
     * Aimed at one square the answer is the pane's own, already said. Aimed at
     * everything around the player it differs per row, and the screen answers
     * with a two-letter column that says nothing without sight.
     */
    std::string square;
    /** Whether the cursor is on this row. */
    bool selected = false;
};

/**
 * Fire the `on_advanced_inventory` hook for the screen about to wait for a key.
 *
 * Params handed to Lua: `source` and `destination` (each `{ area, vehicle }`),
 * `count` (how many item rows the active pane holds), `cursor` (the 1-based
 * position of the selection among those) and `entry` (the selected row as
 * `{ text, category, square }`).
 *
 * `cursor` and `entry` are left unset rather than zeroed when nothing is
 * selected -- an empty pane, or a filter that matched nothing -- so a handler
 * cannot read a selection out of a pane that has none.
 *
 * Fired once per input round rather than once per screen, so moving the cursor
 * fires it again and the Lua side says only what changed. Only the selected row
 * is handed over, for the reason the plain inventory hands over one: a pane can
 * hold hundreds and copying them all on every keypress would cost more than
 * drawing the screen does.
 *
 * The overload taking the panes is the one the screen calls, and it is the only
 * part of this a test cannot reach: a pane needs a player, a map and a curses
 * window. It reads the rows out of the panes and hands them to the overload
 * below, which is where every decision about what the player hears is made.
 *
 * Not fired while the screen is working through a move of its own, when it
 * takes no key and there is nothing to answer.
 */
void fire_on_advanced_inventory( const advanced_inventory_pane &source,
                                 const advanced_inventory_pane &destination,
                                 const std::array<advanced_inv_area, NUM_AIM_LOCATIONS> &squares );
void fire_on_advanced_inventory( const advanced_inventory_place &source,
                                 const advanced_inventory_place &destination,
                                 const std::vector<advanced_inventory_line> &lines );

} // namespace cata
