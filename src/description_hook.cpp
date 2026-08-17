#include "description_hook.h"

#include "avatar.h"
#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "creature.h"
#include "game.h"
#include "map.h"
#include "mapdata.h"

#include <string>

namespace cata
{

namespace
{

// The thing the screen is describing, described. Which of the three it asks about
// is the screen's own choice; whether there is anything of that kind to describe is
// the character's sight, and an empty answer is what the screen prints its own
// sentence for.
std::string description_of( const tripoint_bub_ms &p, const std::string &target )
{
    avatar &you = get_avatar();
    map &here = get_map();

    if( target == "creature" ) {
        const Creature *critter = g->critter_at( p, true );
        if( critter == nullptr || !you.sees( *critter ) ) {
            return std::string();
        }
        return critter->extended_description();
    }

    if( target == "furniture" ) {
        if( !you.sees( p ) || !here.has_furn( p ) ) {
            return std::string();
        }
        return here.furn( p ).obj().extended_description();
    }

    if( !you.sees( p ) ) {
        return std::string();
    }
    return here.ter( p ).obj().extended_description();
}

} // namespace

void fire_on_description( const tripoint_bub_ms &p, const std::string &target,
                          const std::string &action )
{
    if( action == "TIMEOUT" || action == "ERROR" || action == "ANY_INPUT" ) {
        return;
    }
    if( !has_hooks( "on_description" ) ) {
        return;
    }

    run_hooks( "on_description", [&]( sol::table & params ) {
        params["target"] = target;
        params["text"] = description_of( p, target );
        params["signage"] = get_map().get_signage( p );
    } );
}

} // namespace cata
