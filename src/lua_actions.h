#pragma once

#include <string>
#include <vector>

class input_context;

namespace cata::lua_actions
{

/**
 * An action a mod has added to the default game mode.
 *
 * `name` is what the keybindings screen shows; it is never empty, falling back
 * to the id.
 */
struct entry {
    std::string id;
    std::string name;
};

/**
 * Add an action, to be registered into the default mode input context every
 * time that context is built. Re-registering an existing id replaces it, so
 * reloading a mod's scripts cannot accumulate duplicates.
 *
 * Rejected with a debugmsg: an empty id, and an id the game itself already
 * uses -- that one would give the mod's key the built-in action instead, since
 * `input_context::input_to_action` returns the first registered action bound to
 * the key and `look_up_action` would then resolve it to a real `action_id`.
 */
void register_action( const std::string &id, const std::string &name );

/**
 * Drop every entry. Called whenever the Lua state is built or torn down, so a
 * world loaded without the mod does not inherit its actions.
 */
void clear_actions();

const std::vector<entry> &get_actions();

/** Register every entry into `ctxt`. Called while that context is built. */
void register_all( input_context &ctxt );

/**
 * Fire the `on_action` hook for an action string that is about to be resolved
 * into an `action_id`.
 *
 * Params handed to Lua: `action`, the action string as registered.
 *
 * Returns true when a hook claimed the action by returning false, in which case
 * the caller must not resolve it any further. Hooks run highest priority first
 * and stop at the first claim.
 *
 * This lives outside `handle_action.cpp` so it can be tested without driving the
 * game's main loop.
 */
bool run_on_action_hook( const std::string &action );

} // namespace cata::lua_actions
