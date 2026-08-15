#pragma once

#include <functional>
#include <string_view>

#include "catalua_sol_fwd.h"

namespace cata
{

struct lua_state;

struct hook_opts {
    bool exit_early = false;
    lua_state *state = nullptr;
};

/// Run Lua hooks registered with given name.
/// Register hooks with an empty table in `init_global_state_tables` first.
///
/// Hooks are registered in Lua via `table.insert( game.hooks.<hook_name>, ... )`.
/// Each hook entry can be either:
/// - legacy function: `function( params ) ... end`
/// - table: `{ mod_id = "...", priority = 10, fn = function( params ) ... end }`
///
/// During execution, `params.results` is a table shared by all hooks, and `params.prev`
/// contains the previous hook's return value.
/// Returns `params.results`.
auto run_hooks( std::string_view hook_name,
                std::function < auto( sol::table &params ) -> void > init = nullptr,
const hook_opts &opts = {} ) -> sol::table;

/// Return whether a hook currently has registered entries without building params/results tables.
auto has_hooks( std::string_view hook_name, const hook_opts &opts = {} ) -> bool;

/// Which of two states runs a hook: the one that has a handler for it, and
/// `world` when both do. Either may be null.
///
/// Split out of the lookup so that the choice can be asserted without a world
/// and without the accessibility layer's own state having been built, and
/// because getting it wrong is inaudible: a state that answers a hook while
/// holding no handler for it swallows the firing instead of passing it on.
auto pick_hook_state( std::string_view hook_name, lua_state *world, lua_state *boot ) -> lua_state *;

/// Returns the hook results directly, to simplify.
auto get_hook_results( const sol::table &hook_results ) -> std::vector<sol::object>;

/// Define all hooks that are used in the game.
void define_hooks( lua_state &state );

} // namespace cata
