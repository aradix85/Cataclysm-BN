#include "catalua_bindings.h"

#include "access_places.h"
#include "access_zone.h"
#include "catalua_bindings_utils.h"
#include "catalua_luna.h"
#include "catalua_luna_doc.h"
#include "character.h"
#include "creature.h"
#include "map.h"
#include "mapdata.h"
#include "sounds.h"
#include "units_probability.h"

// Perception queries, exposed so that script can ask what the character can
// actually perceive rather than what happens to be drawn on screen.
//
// The game shows all of this in its own look-around window. Anything it shows
// and script cannot reach is invisible to a self-voicing layer, however
// complete that layer believes itself to be -- which is exactly how infrared
// goggles ended up worthless to a blind player while the mechanic worked fine.
//
// Deliberately free functions in a library of their own rather than methods on
// `map`: none of them belongs to one object, several take a Creature and a
// Character together, and a new file cannot conflict with upstream.

namespace cata::detail
{

void reg_perception( sol::state &lua )
{
    luna::userlib lib = luna::begin_lib( lua, "perception" );

    DOC( "Move cost of a map square, counting terrain, furniture and any vehicle. "
         "0 means impassable. This is what the game shows as 'Impassable' or 'Move cost: N'." );
    luna::set_fx( lib, "move_cost_at", []( const tripoint_bub_ms & p ) -> int {
        return get_map().move_cost( p );
    } );

    DOC( "Whether a map square has a floor. False means stepping there is a fall." );
    luna::set_fx( lib, "has_floor_at", []( const tripoint_bub_ms & p ) -> bool {
        return get_map().has_floor( p );
    } );

    DOC( "How hard a map square is to see through, as a percentage. This is concealment, "
         "not protection from gunfire -- see block_unaimed_chance_at for that." );
    luna::set_fx( lib, "coverage_at", []( const tripoint_bub_ms & p ) -> int {
        return get_map().coverage( p );
    } );

    DOC( "Chance that what is on a map square stops an unaimed shot, as a percentage. "
         "Furniture takes precedence over terrain, as it does in the game's own display." );
    luna::set_fx( lib, "block_unaimed_chance_at", []( const tripoint_bub_ms & p ) -> int {
        map &here = get_map();
        const map_bash_info &bash = here.has_furn( p ) ? here.furn( p ).obj().bash : here.ter( p ).obj().bash;
        if( !bash.ranged ) {
            return 0;
        }
        // Stored as a one-in-a-million probability; the display, and a spoken
        // report, want whole percent.
        return units::to_one_in_million<int>( bash.ranged->block_unaimed_chance ) / 10000;
    } );

    DOC( "Text written on a map square -- a sign, a road marking. Empty when there is none." );
    luna::set_fx( lib, "signage_at", []( const tripoint_bub_ms & p ) -> std::string {
        return get_map().get_signage( p );
    } );

    DOC( "Description of the terrain of a map square, as the look-around window gives it." );
    luna::set_fx( lib, "ter_description_at", []( const tripoint_bub_ms & p ) -> std::string {
        return get_map().ter( p ).obj().description.translated();
    } );

    DOC( "Description of the furniture of a map square. Empty when there is no furniture." );
    luna::set_fx( lib, "furn_description_at", []( const tripoint_bub_ms & p ) -> std::string {
        map &here = get_map();
        if( !here.has_furn( p ) ) {
            return std::string();
        }
        return here.furn( p ).obj().description.translated();
    } );

    DOC( "What is heard from a map square, as a sentence. Empty when nothing was heard there. "
         "A sound is perceivable with no line of sight at all, so it is the one channel that "
         "reaches through walls." );
    luna::set_fx( lib, "sound_at", []( const tripoint_bub_ms & p ) -> std::string {
        return sounds::sound_at( p );
    } );

    DOC( "Positions of footsteps heard this turn. Heard, not seen: these have no creature "
         "attached and may be behind a wall." );
    luna::set_fx( lib, "footstep_markers", []() -> std::vector<tripoint_bub_ms> {
        return sounds::get_footstep_markers();
    } );

    DOC( "Whether the character detects this creature by infrared -- body heat through "
         "darkness, and through walls for some sensors. True while ordinary sight fails." );
    luna::set_fx( lib, "sees_with_infrared",
    []( const Character & who, const Creature & critter ) -> bool {
        return who.sees_with_infrared( critter );
    } );

    DOC( "Whether the character detects this creature by a special sense: electroreception, "
         "ground sonar, eyebot marking, antennae. Several of these reach through walls." );
    luna::set_fx( lib, "sees_with_specials",
    []( const Character & who, const Creature & critter ) -> bool {
        return who.sees_with_specials( critter );
    } );

    DOC( "How a creature detected by infrared reads, as the game itself words it." );
    luna::set_fx( lib, "describe_infrared", []( const Creature & critter ) -> std::vector<std::string> {
        std::vector<std::string> buf;
        critter.describe_infrared( buf );
        return buf;
    } );

    DOC( "How a creature detected by a special sense reads, as the game itself words it." );
    luna::set_fx( lib, "describe_specials", []( const Creature & critter ) -> std::vector<std::string> {
        std::vector<std::string> buf;
        critter.describe_specials( buf );
        return buf;
    } );

    DOC( "Name of the region a map square belongs to -- 'overgrown cabin', 'forest'. This is the "
         "area name the game's own look-around window prints above a square, and it is reachable "
         "no other way from script: the overmap terrain's id is bound and the terrain itself is "
         "not." );
    luna::set_fx( lib, "area_name_at", []( const tripoint_bub_ms & p ) -> std::string {
        return cata::access::area_name_at( p );
    } );

    DOC( "The place the character is standing in, as a whole: its name, its size in region "
         "tiles, how far it runs from her in each direction, and how far she has already been. "
         "The difference between the last two is the part of it she has never seen, which is "
         "the one thing here no screen in the game answers. Returns a table with `name`, "
         "`tiles_wide`, `tiles_high`, `reach_north`/`east`/`south`/`west` and the matching "
         "`seen_*`, all in steps." );
    luna::set_fx( lib, "zone_around_player", []( sol::this_state s ) -> sol::table {
        const cata::access::zone_report zone = cata::access::zone_around_player();

        sol::state_view lua( s );
        sol::table out = lua.create_table( 0, 11 );
        out["name"] = zone.name;
        out["tiles_wide"] = zone.tiles_wide;
        out["tiles_high"] = zone.tiles_high;
        out["reach_north"] = zone.reach_north;
        out["reach_east"] = zone.reach_east;
        out["reach_south"] = zone.reach_south;
        out["reach_west"] = zone.reach_west;
        out["seen_north"] = zone.seen_north;
        out["seen_east"] = zone.seen_east;
        out["seen_south"] = zone.seen_south;
        out["seen_west"] = zone.seen_west;
        return out;
    } );

    luna::finalize_lib( lib );
}

} // namespace cata::detail
