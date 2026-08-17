#include "access_zone.h"

#include "access_places.h"
#include "avatar.h"
#include "coordinates.h"
#include "game_constants.h"
#include "map_memory.h"

#include <algorithm>
#include <string>

namespace cata::access
{

namespace
{

// How many region tiles are walked in each direction before the answer stops being
// about this place. Four is two full screens' worth and larger than all but the
// biggest buildings; past that a name repeating is a landscape rather than a place.
constexpr int max_tiles = 4;

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

} // namespace cata::access
