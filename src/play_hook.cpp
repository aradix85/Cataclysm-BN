#include "play_hook.h"

#include "catalua_hooks.h"
#include "catalua_sol.h"

namespace cata
{

void fire_on_play_input()
{
    if( !has_hooks( "on_play_input" ) ) {
        return;
    }

    run_hooks( "on_play_input", []( sol::table & ) {} );
}

} // namespace cata
