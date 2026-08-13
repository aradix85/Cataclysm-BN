#include "catalua_bindings.h"

#include <algorithm>
#include <string>
#include <vector>

#include "catalua_bindings_utils.h"
#include "catalua_luna.h"
#include "catalua_luna_doc.h"

#include "input.h"
#include "translations.h"

// An input_context owned by Lua. Handing a script its own context is what lets
// it read keys in a loop of its own -- browsing a list, say -- instead of one
// command per keypress. Such a loop costs no game time.
//
// A context built here is a different object from the one the main loop uses,
// so registering an action in it does NOT make that key work in ordinary play.
// That is what gapi.register_default_mode_action and the on_action hook are for.

void cata::detail::reg_input_context( sol::state &lua )
{
    sol::usertype<input_context> ut =
        luna::new_usertype<input_context>(
            lua,
            luna::no_bases,
            luna::constructors <
            input_context( const std::string & )
            > ()
        );

    DOC( "Register an action this context will report. Only registered actions are ever returned by `handle_input`. The optional second argument is the name shown in the keybindings screen." );
    luna::set_fx( ut, "register_action", []( input_context & ctxt, const std::string & action,
    sol::optional<std::string> name ) -> void {
        if( name ) {
            ctxt.register_action( action, to_translation( *name ) );
        } else {
            ctxt.register_action( action );
        }
    } );

    DOC( "Register the eight movement directions plus their diagonals, so `handle_input` reports them as one of the direction actions." );
    luna::set_fx( ut, "register_directions", []( input_context & ctxt ) -> void {
        ctxt.register_directions();
    } );

    DOC( "Whether an action has been registered in this context." );
    luna::set_fx( ut, "is_action_registered", []( const input_context & ctxt,
    const std::string & action ) -> bool {
        // input_context::is_action_registered is compiled only on Android; this
        // copying accessor is the portable one.
        const std::vector<std::string> registered = ctxt.get_registered_actions_copy();
        return std::ranges::find( registered, action ) != registered.end();
    } );

    DOC( "Wait for a keypress and return the action it is bound to, or an error string for an unbound key. With a timeout in milliseconds, returns `TIMEOUT` when nothing was pressed. Blocks the game while it waits." );
    luna::set_fx( ut, "handle_input", []( input_context & ctxt,
    sol::optional<int> timeout ) -> std::string {
        return timeout ? ctxt.handle_input( *timeout ) : ctxt.handle_input();
    } );
}
