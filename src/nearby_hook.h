#pragma once

#include "coordinates.h"

#include <string>
#include <vector>

class map_item_stack;

namespace cata
{

/**
 * One row of the game's list of what is lying around the player: the item as the
 * screen names it, how many of it there are, which square it is on relative to the
 * player, and the heading it sits under while the list is grouped.
 */
struct nearby_row {
    std::string text;
    std::string category;
    int count = 1;
    tripoint_rel_ms offset;
    bool selected = false;
};

/**
 * Fire the `on_nearby_items` hook for the list the item key opens, which is about
 * to wait for a key.
 *
 * Params handed to Lua: `count` (how many rows the filter leaves), `cursor` (the
 * 1-based position of the selection among them) and `entry` as
 * `{ text, category, count, dx, dy, dz }`. `cursor` and `entry` are left unset
 * rather than zeroed when the list is empty or a filter leaves nothing, so a
 * handler cannot read a selection out of a list that has none.
 *
 * A row's square is handed over as an offset from the player rather than as a
 * position, because that is what it is: the screen draws a trail to it and the
 * layer says a distance and a compass point (P3). Offsets are what the list itself
 * stores.
 *
 * The headings are not rows here. The screen inserts one above each group and
 * counts it in its own cursor, so a position spoken from that cursor would drift
 * further out with every group passed; the heading travels with the row it belongs
 * to instead.
 *
 * Fired once per input round, so moving the selection fires again -- which is why
 * the Lua side keeps the previous state and says only what changed. Not fired for
 * the input layer's sentinels.
 */
void fire_on_nearby_items( const std::vector<nearby_row> &rows, const std::string &action );

/**
 * The same, for the screen: turns the list it is showing into rows.
 *
 * `page` is which of a stack's squares the selection is paging through, since one
 * row can stand for the same item lying in several places. `grouped` says whether
 * the screen is currently sorted into categories, because it draws a heading only
 * then; sorted by distance a heading would change with nearly every row and say
 * nothing about where anything is.
 */
void fire_on_nearby_items( const std::vector<map_item_stack> &filtered,
                           const map_item_stack *active, int page, bool grouped,
                           const std::string &action );

} // namespace cata
