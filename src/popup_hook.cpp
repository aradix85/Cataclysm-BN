#include "popup_hook.h"

#include "cata_utility.h"
#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "init.h"
#include "input.h"

namespace cata
{

namespace
{
bool running_on_query_popup_hook = false;
} // namespace

bool is_running_on_query_popup_hook()
{
    return running_on_query_popup_hook;
}

void run_on_query_popup_hook( const std::string &category, const std::string &text,
                              const std::vector<std::string> &option_actions,
                              const std::size_t cursor )
{
    // Popups exist before a world is loaded and after it is unloaded, when there
    // is no Lua state at all; `has_hooks` would dereference a null pointer.
    if( running_on_query_popup_hook || !DynamicDataLoader::get_instance().lua ) {
        return;
    }
    if( !has_hooks( "on_query_popup" ) ) {
        return;
    }

    restore_on_out_of_scope<bool> restore_running( running_on_query_popup_hook );
    running_on_query_popup_hook = true;

    run_hooks( "on_query_popup", [&]( sol::table & params ) {
        params["text"] = text;
        params["category"] = category;

        // The same lookup the popup uses to label its own buttons, so a handler
        // never has to guess what "YES" reads as in the player's language.
        const input_context ctxt( category );
        sol::state_view lua( params.lua_state() );
        sol::table options = lua.create_table( static_cast<int>( option_actions.size() ), 0 );
        for( std::size_t i = 0; i < option_actions.size(); ++i ) {
            sol::table option = lua.create_table( 0, 2 );
            option["id"] = option_actions[i];
            option["name"] = ctxt.get_action_name( option_actions[i] );
            options[i + 1] = option;
        }
        params["options"] = options;

        // Left unset rather than zero when there is nothing to select, so a
        // handler cannot read a selection out of a popup that has none.
        if( cursor < option_actions.size() ) {
            params["cursor"] = cursor + 1;
        }
    } );
}

} // namespace cata
