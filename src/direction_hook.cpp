#include "direction_hook.h"

#include "catalua_hooks.h"
#include "catalua_sol.h"

namespace cata
{

void fire_on_direction_prompt( const std::string &message, const bool allow_vertical,
                               const std::string &action )
{
    // The input layer answers these when nothing was pressed or nothing matched.
    // They carry no round of their own, and firing on them would say the question
    // again for a key the prompt never saw.
    if( action == "TIMEOUT" || action == "ERROR" || action == "ANY_INPUT" ) {
        return;
    }
    if( !has_hooks( "on_direction_prompt" ) ) {
        return;
    }

    run_hooks( "on_direction_prompt", [&message, allow_vertical, &action]( sol::table & params ) {
        params["text"] = message;
        params["vertical"] = allow_vertical;
        params["action"] = action;
    } );
}

} // namespace cata
