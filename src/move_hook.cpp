#include "move_hook.h"

#include "avatar.h"
#include "catalua_coord.h"
#include "catalua_hooks.h"
#include "catalua_sol.h"

namespace cata
{

void run_on_player_move_refused_hook( avatar &you, const tripoint_bub_ms &from,
                                      const tripoint_bub_ms &to, const std::string &obstacle )
{
    if( !has_hooks( "on_player_move_refused" ) ) {
        return;
    }

    run_hooks( "on_player_move_refused", [&you, &from, &to, &obstacle]( sol::table & params ) {
        params["player"] = &you;
        params["from"] = detail::lua_coords::to_lua( from );
        params["to"] = detail::lua_coords::to_lua( to );
        params["obstacle"] = obstacle;
    } );
}

} // namespace cata
