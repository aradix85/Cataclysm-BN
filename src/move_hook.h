#pragma once

#include "coordinates.h"

#include <string>

class avatar;

namespace cata
{

/**
 * Fire the `on_player_move_refused` hook for a move the game has just refused
 * while telling the player nothing about it.
 *
 * Params handed to Lua: `player`, `from`, `to` and `obstacle` -- the name the game
 * itself would use for whatever is in the way, a vehicle part included.
 *
 * The refusals the game does report -- bumping into something while blind or
 * stunned, a locked door, a barred door -- reach Lua as messages instead, so this
 * fires only where the game says nothing at all. That is why the call sits in the
 * final branch of the invalid-move chain in `avatar_action::move` rather than ahead
 * of it: a mod acting on both would say the same thing twice.
 */
void run_on_player_move_refused_hook( avatar &you, const tripoint_bub_ms &from,
                                      const tripoint_bub_ms &to, const std::string &obstacle );

} // namespace cata
