#pragma once

#include <string>

namespace cata
{

/**
 * Fire the `on_world_loading` hook at the point the game commits to a world and
 * begins building it, before any of the work that makes it wait.
 *
 * Params handed to Lua: `world`, the name of the world being brought up, and
 * `reading_data`, false when the world's content is already in memory and the
 * game is only rebuilding its state.
 *
 * A mod cannot answer this hook: the state it would live in is destroyed and
 * rebuilt by the load this announces. It exists for anything that outlives a
 * world -- which is why it fires before the load rather than after it, where
 * every other signal about a world already sits. Loading a world reads every
 * JSON file the mod list names and then generates the map around the player,
 * and neither step reports progress anywhere a program can hear it, so without
 * this the wait is indistinguishable from a game that has stopped.
 */
void run_on_world_loading_hook( const std::string &world_name, bool reading_data );

} // namespace cata
