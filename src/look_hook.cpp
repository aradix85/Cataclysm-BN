#include "look_hook.h"

#include "catalua_coord.h"
#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "map.h"

#include <string>

namespace cata
{

std::string sight_of( const visibility_type visibility )
{
    switch( visibility ) {
        case VIS_CLEAR:
            return "clear";
        case VIS_LIT:
            return "lit";
        case VIS_DARK:
            return "dark";
        case VIS_BOOMER:
            return "blur";
        case VIS_BOOMER_DARK:
            return "blur_dark";
        case VIS_HIDDEN:
            break;
    }
    return "hidden";
}

void fire_on_look_around( const tripoint_bub_ms &cursor, const bool show_window,
                          const bool peeking, const std::string &action )
{
    if( !show_window ) {
        return;
    }
    if( action == "TIMEOUT" || action == "ERROR" || action == "ANY_INPUT" ) {
        return;
    }
    if( !has_hooks( "on_look_around" ) ) {
        return;
    }

    map &here = get_map();
    // A square off the edge of the loaded map has no light and no contents to ask
    // about, and the screen prints nothing for it either.
    const visibility_type visibility = here.inbounds( cursor )
                                       ? here.get_visibility( here.apparent_light_at( cursor,
                                           here.get_visibility_variables_cache() ),
                                           here.get_visibility_variables_cache() )
                                       : VIS_HIDDEN;

    run_hooks( "on_look_around", [&]( sol::table & params ) {
        // catalua_coord.h is included for this line and not for a type: it carries
        // the push customisation for every coordinate, found by argument lookup.
        // Without it a coordinate is handed over as opaque userdata that reads as
        // present and is refused by every binding it is passed to.
        params["cursor"] = cursor;
        params["sight"] = sight_of( visibility );
        params["peeking"] = peeking;
        params["action"] = action;
    } );
}

} // namespace cata
