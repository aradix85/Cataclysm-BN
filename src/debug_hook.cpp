#include "debug_hook.h"

#include "cata_utility.h"
#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "init.h"

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
    // Errors are reported before a world is loaded and after it is unloaded,
    // when there is no Lua state at all; `has_hooks` would dereference a null
    // pointer.
    if( running_on_debugmsg_hook || !DynamicDataLoader::get_instance().lua ) {
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
