#include "access_zone.h"

#include "access_places.h"
#include "avatar.h"
#include "coordinates.h"
#include "game_constants.h"
#include "map.h"
#include "map_memory.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "mapdata.h"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace cata::access
{

namespace
{

// How many region tiles are walked in each direction before the answer stops being
// about this place. Four is two full screens' worth and larger than all but the
// biggest buildings; past that a name repeating is a landscape rather than a place.
constexpr int max_tiles = 4;

// How far the known-places answer looks, in region tiles. One region block, which
// is the reach the game's own search has: the two answers are about the same stock
// or they disagree about a world she cannot check by looking.
constexpr int search_radius = OMAPX;

// How far the same name runs from a starting tile, in tiles, along one direction.
int run_of( const tripoint_abs_omt &from, const std::string &name, const int dx, const int dy )
{
    int found = 0;
    for( int step = 1; step <= max_tiles; ++step ) {
        const tripoint_abs_omt at( from.x() + dx * step, from.y() + dy * step, from.z() );
        if( area_name_at( at ) != name ) {
            break;
        }
        found = step;
    }
    return found;
}

} // namespace

zone_report zone_around_player()
{
    avatar &you = get_avatar();
    const tripoint_abs_omt centre = you.abs_omt_pos();

    zone_report out;
    out.name = area_name_at( centre );

    const int north = run_of( centre, out.name, 0, -1 );
    const int east = run_of( centre, out.name, 1, 0 );
    const int south = run_of( centre, out.name, 0, 1 );
    const int west = run_of( centre, out.name, -1, 0 );

    out.tiles_wide = west + east + 1;
    out.tiles_high = north + south + 1;

    // The zone's own edges, as squares, and her own square inside them.
    const tripoint_abs_ms here = you.abs_pos();
    const tripoint_abs_ms top_left = project_to<coords::ms>( tripoint_abs_omt(
                                         centre.x() - west, centre.y() - north, centre.z() ) );
    const tripoint_abs_ms bottom_right = project_to<coords::ms>( tripoint_abs_omt(
            centre.x() + east, centre.y() + south, centre.z() ) );
    const int left = top_left.x();
    const int top = top_left.y();
    const int right = bottom_right.x() + SEEX * 2 - 1;
    const int bottom = bottom_right.y() + SEEY * 2 - 1;

    out.reach_north = here.y() - top;
    out.reach_south = bottom - here.y();
    out.reach_west = here.x() - left;
    out.reach_east = right - here.x();

    // What she has already laid eyes on, as the box holding every remembered square
    // inside this zone. A box rather than a reach along each line, because a place
    // is walked around corners and a straight line out of her own square would
    // report a wall as the end of what she knows.
    you.prepare_map_memory_region( tripoint_abs_ms( left, top, here.z() ),
                                   tripoint_abs_ms( right, bottom, here.z() ) );

    int seen_left = here.x();
    int seen_right = here.x();
    int seen_top = here.y();
    int seen_bottom = here.y();
    for( int y = top; y <= bottom; ++y ) {
        for( int x = left; x <= right; ++x ) {
            if( you.get_memorized_tile( tripoint_abs_ms( x, y, here.z() ) ).tile.empty() ) {
                continue;
            }
            seen_left = std::min( seen_left, x );
            seen_right = std::max( seen_right, x );
            seen_top = std::min( seen_top, y );
            seen_bottom = std::max( seen_bottom, y );
        }
    }

    out.seen_north = here.y() - seen_top;
    out.seen_south = seen_bottom - here.y();
    out.seen_west = here.x() - seen_left;
    out.seen_east = seen_right - here.x();

    return out;
}

std::vector<zone_exit> exits_in_zone()
{
    avatar &you = get_avatar();
    map &here = get_map();
    const zone_report zone = zone_around_player();
    const tripoint_bub_ms at = you.bub_pos();

    std::vector<zone_exit> out;
    // The zone's own bounds, clipped to the loaded map on the way in: a square the
    // game has not built cannot be asked what is on it.
    for( int dy = -zone.reach_north; dy <= zone.reach_south; ++dy ) {
        for( int dx = -zone.reach_west; dx <= zone.reach_east; ++dx ) {
            const tripoint_bub_ms p( at.x() + dx, at.y() + dy, at.z() );
            if( !here.inbounds( p ) || ( dx == 0 && dy == 0 ) ) {
                continue;
            }

            std::string kind;
            if( here.has_flag_ter( TFLAG_GOES_UP, p ) ) {
                kind = "up";
            } else if( here.has_flag_ter( TFLAG_GOES_DOWN, p ) ) {
                kind = "down";
            } else if( here.has_flag_ter( "DOOR", p ) ) {
                // By name rather than by one of the fast flags: the two staircase
                // flags have one and a door does not, which is upstream's choice
                // and not something to work around.
                kind = "door";
            } else {
                continue;
            }

            // Seen now, or seen once and remembered. The order matters for cost:
            // line of sight is the expensive question and is asked last, of the few
            // squares that turned out to be a way out at all.
            if( you.get_memorized_tile( bub_to_abs( p ) ).tile.empty() && !you.sees( p ) ) {
                continue;
            }

            zone_exit exit;
            exit.name = here.ter( p ).obj().name();
            exit.kind = kind;
            exit.dx = dx;
            exit.dy = dy;
            out.push_back( exit );
        }
    }

    // Nearest first, because that is the order she would walk them in, and diagonal
    // distance the way the game counts it rather than as a straight line.
    std::ranges::sort( out, []( const zone_exit & a, const zone_exit & b ) {
        return std::max( std::abs( a.dx ), std::abs( a.dy ) ) <
               std::max( std::abs( b.dx ), std::abs( b.dy ) );
    } );
    return out;
}

std::vector<known_place> known_places()
{
    const tripoint_abs_omt centre = get_avatar().abs_omt_pos();
    overmapbuffer &buffer = ACTIVE_OVERMAP_BUFFER;

    // The same reach the game's own search has, so that the two answers are about
    // the same stock: one region block in every direction, and seen tiles only.
    std::map<std::string, known_place> found;
    for( int dy = -search_radius; dy <= search_radius; ++dy ) {
        for( int dx = -search_radius; dx <= search_radius; ++dx ) {
            const tripoint_abs_omt p( centre.x() + dx, centre.y() + dy, centre.z() );
            if( !buffer.seen( p ) ) {
                continue;
            }

            const std::string name = area_name_at( p );
            if( name.empty() ) {
                continue;
            }

            const int away = std::max( std::abs( dx ), std::abs( dy ) );
            auto it = found.find( name );
            if( it == found.end() ) {
                found.emplace( name, known_place{ name, 1, dx, dy } );
                continue;
            }

            ++it->second.count;
            if( away < std::max( std::abs( it->second.dx ), std::abs( it->second.dy ) ) ) {
                it->second.dx = dx;
                it->second.dy = dy;
            }
        }
    }

    std::vector<known_place> out;
    out.reserve( found.size() );
    for( const auto &entry : found ) {
        out.push_back( entry.second );
    }

    // Nearest first: the question behind this is where to go next, and the answer
    // to that is ordered by how far it is rather than by how much of it there is.
    std::ranges::sort( out, []( const known_place & a, const known_place & b ) {
        return std::max( std::abs( a.dx ), std::abs( a.dy ) ) <
               std::max( std::abs( b.dx ), std::abs( b.dy ) );
    } );
    return out;
}

} // namespace cata::access
