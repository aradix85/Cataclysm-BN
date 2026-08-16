#include "world_loading_hook.h"

#include "catalua_hooks.h"
#include "catalua_sol.h"

namespace cata
{

void run_on_world_loading_hook( const std::string &world_name, bool reading_data )
{
    if( !has_hooks( "on_world_loading" ) ) {
        return;
    }

    run_hooks( "on_world_loading", [&world_name, reading_data]( sol::table & params ) {
        params["world"] = world_name;
        params["reading_data"] = reading_data;
    } );
}

} // namespace cata
