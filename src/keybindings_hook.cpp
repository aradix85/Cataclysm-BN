#include "keybindings_hook.h"

#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "input.h"
#include "translations.h"

namespace cata
{

void fire_on_keybindings( const input_context &ctxt, const std::string &category,
                          const std::vector<std::string> &visible,
                          const std::size_t scroll_offset )
{
    // No check for a world's Lua state. This screen opens from the world
    // picker and the load list as readily as from the inventory, and
    // `resolve_hook_state` answers with the layer's own state where there is
    // no world.
    if( !has_hooks( "on_keybindings" ) ) {
        return;
    }

    run_hooks( "on_keybindings", [&]( sol::table & params ) {
        // The context being described, not the one this screen runs in: what a
        // player wants to know is whose keys these are, and it is also what
        // makes returning to the screen underneath read as arriving somewhere.
        params["category"] = category;
        // The same string the border draws, so the screen is called what the
        // game calls it.
        params["title"] = _( "Keybindings" );
        params["count"] = static_cast<int>( visible.size() );

        if( scroll_offset >= visible.size() ) {
            // An empty filter result, or an offset the screen has not yet
            // clamped. Leaving both unset rather than zeroed means a handler
            // reads "nothing to report" instead of announcing a row that is
            // not there.
            return;
        }
        const std::string &action_id = visible[scroll_offset];

        params["cursor"] = static_cast<int>( scroll_offset ) + 1;
        sol::state_view lua( params.lua_state() );
        sol::table entry = lua.create_table( 0, 2 );
        entry["text"] = ctxt.get_action_name( action_id );
        // What the screen prints in its second column: the keys bound to this
        // action, or that it is unbound, in the game's own words.
        entry["column"] = ctxt.get_desc( action_id );
        params["entry"] = entry;
    } );
}

} // namespace cata
