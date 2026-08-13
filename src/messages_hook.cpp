#include "messages_hook.h"

#include "cata_utility.h"
#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "enums.h"

namespace cata
{

namespace
{
bool running_on_add_msg_hook = false;
} // namespace

bool is_running_on_add_msg_hook()
{
    return running_on_add_msg_hook;
}

void run_on_add_msg_hook( const std::string &text, const game_message_type type )
{
    if( running_on_add_msg_hook || !has_hooks( "on_add_msg" ) ) {
        return;
    }

    restore_on_out_of_scope<bool> restore_running( running_on_add_msg_hook );
    running_on_add_msg_hook = true;

    run_hooks( "on_add_msg", [&text, type]( sol::table & params ) {
        params["text"] = text;
        params["type"] = type;
    } );
}

} // namespace cata
