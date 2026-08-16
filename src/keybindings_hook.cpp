#include "keybindings_hook.h"

#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "input.h"
#include "translations.h"

#include <algorithm>

namespace cata
{

list_position clamp_selection( list_position at, const std::size_t size, const std::size_t height )
{
    if( size == 0 ) {
        return list_position{ 0, 0 };
    }
    at.selected = std::min( at.selected, size - 1 );

    // The window follows the selection: far enough back to show it, and never
    // further down the list than the last full page, so the rows at the end are
    // reachable without the view running off into nothing.
    if( height == 0 ) {
        at.offset = at.selected;
        return at;
    }
    if( at.selected < at.offset ) {
        at.offset = at.selected;
    } else if( at.selected >= at.offset + height ) {
        at.offset = at.selected - height + 1;
    }
    const std::size_t last_offset = size > height ? size - height : 0;
    at.offset = std::min( at.offset, last_offset );
    return at;
}

list_position move_selection( list_position at, const int delta, const std::size_t size,
                              const std::size_t height, const bool wrap )
{
    if( size == 0 ) {
        return list_position{ 0, 0 };
    }
    const auto signed_size = static_cast<long long>( size );
    auto target = static_cast<long long>( std::min( at.selected, size - 1 ) ) + delta;

    if( wrap ) {
        // Both ends, so a step is always answered by a row -- a keypress that
        // moves nothing is indistinguishable from a dead keyboard.
        target = ( ( target % signed_size ) + signed_size ) % signed_size;
    } else {
        target = std::max( 0LL, std::min( target, signed_size - 1 ) );
    }

    at.selected = static_cast<std::size_t>( target );
    return clamp_selection( at, size, height );
}

void fire_on_keybindings( const input_context &ctxt, const std::string &category,
                          const std::vector<std::string> &visible,
                          const std::size_t selected )
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

        if( selected >= visible.size() ) {
            // The filter left nothing, or a selection the screen has not yet
            // put back inside the list. Leaving both params unset rather than
            // zeroed means a handler reads "nothing to report" instead of
            // naming a row that is not there.
            return;
        }
        const std::string &action_id = visible[selected];

        params["cursor"] = static_cast<int>( selected ) + 1;
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
