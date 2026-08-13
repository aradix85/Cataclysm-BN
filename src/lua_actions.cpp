#include "lua_actions.h"

#include <algorithm>

#include "action.h"
#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "debug.h"
#include "input.h"
#include "translations.h"

namespace cata::lua_actions
{

namespace
{
std::vector<entry> &storage()
{
    static std::vector<entry> entries;
    return entries;
}
} // namespace

void register_action( const std::string &id, const std::string &name )
{
    if( id.empty() ) {
        debugmsg( "Lua action id must not be empty." );
        return;
    }
    if( look_up_action( id ) != ACTION_NULL ) {
        debugmsg( "Lua action id '%s' is already a built-in game action.", id );
        return;
    }

    const std::string &display_name = name.empty() ? id : name;
    std::vector<entry> &entries = storage();
    const auto found = std::ranges::find_if( entries, [&id]( const entry & e ) {
        return e.id == id;
    } );
    if( found != entries.end() ) {
        found->name = display_name;
        return;
    }
    entries.push_back( entry{ id, display_name } );
}

void clear_actions()
{
    storage().clear();
}

const std::vector<entry> &get_actions()
{
    return storage();
}

void register_all( input_context &ctxt )
{
    for( const entry &e : storage() ) {
        ctxt.register_action( e.id, to_translation( e.name ) );
    }
}

bool run_on_action_hook( const std::string &action )
{
    if( action.empty() || !has_hooks( "on_action" ) ) {
        return false;
    }

    const sol::table results = run_hooks( "on_action", [&action]( sol::table & params ) {
        params["action"] = action;
    }, { .exit_early = true } );

    const sol::optional<bool> allowed = results["allowed"];
    return allowed.has_value() && !*allowed;
}

} // namespace cata::lua_actions
