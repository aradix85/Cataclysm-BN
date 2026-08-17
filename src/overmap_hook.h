#pragma once

#include "coordinates.h"

#include <string>

namespace cata
{

/**
 * The overmap tile under the cursor, as plain values, in the words the screen's
 * own sidebar uses for it.
 *
 * `place` is the description the sidebar prints -- "house in central Springfield"
 * -- and is empty for a tile that has never been seen, which is the sidebar's
 * "Unexplored".
 *
 * `dx`, `dy` and `dz` are the cursor relative to the player's own tile, never an
 * absolute position. The layer says a distance and a compass point (P3), and a
 * coordinate on a map nobody can look at answers nothing.
 *
 * `route` is how many tiles the previewed route to this tile holds, and 0 when
 * there is none. The travel key only previews on its first press: it draws a
 * path, says nothing, and demands a second press to act. Without this the key is
 * answered by silence both when it worked and when no route exists at all.
 *
 * `action` is the action the round answered, empty at the firing that opens the
 * screen.
 */
struct overmap_view {
    std::string place;
    std::string note;
    bool seen = false;
    bool explored = false;
    int dx = 0;
    int dy = 0;
    int dz = 0;
    int route = 0;
    std::string action;
};

/**
 * Fire the `on_overmap` hook for the overmap that is about to wait for a key.
 *
 * Params handed to Lua: `place`, `note`, `seen`, `explored`, `dx`, `dy`, `dz`,
 * `route` and `action`, as described above.
 *
 * Fired once per input round rather than once per screen, so moving the cursor
 * fires it again -- which is why the Lua side keeps the previous tile and says
 * only what changed.
 *
 * Not fired for the input layer's sentinels. The overmap waits for input with a
 * timeout so that it can blink, so it completes a round several times a second
 * whatever the player does, and each one would ask the overmap buffer for a
 * description of the same tile and for the city it lies near. An empty action is
 * not one of those: it is the screen opening, before it has waited for anything,
 * and that is the one firing that has to speak. Mouse edge scrolling moves the
 * cursor on exactly the rounds this drops, and is therefore not spoken.
 *
 * No re-entry guard: the overmap is not what the game raises to report an error,
 * so a failing handler cannot arrive back here the way it can through a popup.
 */
void fire_on_overmap( const overmap_view &view );

/**
 * The same, for the screen: reads the cursor's tile out of the overmap buffer
 * and the route out of the player, and hands over the result.
 */
void fire_on_overmap( const tripoint_abs_omt &cursor, const std::string &action );

} // namespace cata
