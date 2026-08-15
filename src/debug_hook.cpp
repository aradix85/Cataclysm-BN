#include "debug_hook.h"

#include "cata_utility.h"
#include "catalua_hooks.h"
#include "catalua_sol.h"

namespace cata
{

namespace
{
bool running_on_debugmsg_hook = false;
} // namespace

bool is_running_on_debugmsg_hook()
{
    return running_on_debugmsg_hook;
}

void run_on_debugmsg_hook( const std::string &text )
{
    // No check for a world's Lua state. An error is reported before a world is
    // loaded and after it is unloaded -- a failure while loading one is exactly
    // that -- and `resolve_hook_state` answers with the layer's own state there.
    // This screen swallows every key but its own, so a silent one leaves nothing
    // to press at all.
    if( running_on_debugmsg_hook ) {
        return;
    }
    if( !has_hooks( "on_debugmsg" ) ) {
        return;
    }

    restore_on_out_of_scope<bool> restore_running( running_on_debugmsg_hook );
    running_on_debugmsg_hook = true;

    run_hooks( "on_debugmsg", [&text]( sol::table & params ) {
        params["text"] = text;
    } );
}

} // namespace cata
