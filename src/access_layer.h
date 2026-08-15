#pragma once

namespace cata
{

struct lua_state;

/**
 * The accessibility layer's Lua, which the fork owns rather than the mod system.
 *
 * It is not a mod. A mod exists only while a world does — `unload_data()` ends
 * in `lua.reset()` — so as a mod the layer was silent before a world was chosen
 * and silent again the moment one was left, which is the screen a player without
 * sight meets first and last. It would also sit in the mod list, where the one
 * thing that must never be switched off could be switched off.
 *
 * It stays Lua, because a Lua change compiles nothing, which is what makes the
 * wording answerable at the keyboard rather than at a build. It does cost a
 * restart: `reload_lua_code()` reloads the mod scripts only, and `game.add_hook`
 * appends without replacing, so loading this file a second time into a live
 * state would register every hook twice and say everything twice.
 *
 * The layer is loaded into whichever state is alive: the one built here at
 * program start, and again into each world's state as it is built. Hooks then
 * resolve to a single state — the world's while there is one, this one
 * otherwise — so nothing is ever said twice.
 */
namespace access
{

/// Build the layer's own state and load the layer into it. Called once, before
/// the first screen is drawn.
void start();

/// The state built by `start()`, or null before it has run.
auto boot_state() -> lua_state *;

/// Load the layer's scripts into a state that already has the game's bindings
/// and hook tables. Reports through `debugmsg` and never throws: a fault here
/// is the layer failing, and the game has to keep running.
void load_into( lua_state &state );

} // namespace access

} // namespace cata
