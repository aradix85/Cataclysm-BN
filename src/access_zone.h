#pragma once

#include <string>

namespace cata::access
{

/**
 * The place the character is standing in, as a whole rather than as the square
 * under her feet.
 *
 * A region tile is always 24 squares across, but a place -- a station, a mall, a
 * farm -- usually spans several of them under one name, and how far it runs is
 * something a sighted player reads off the screen and the map. There is nothing in
 * the game that answers it in words.
 *
 * `reach_*` is how far the place itself goes from her, in steps. `seen_*` is how
 * far she has been: the map memory holds what she has laid eyes on and keeps it
 * after she leaves, so the difference between the two is the part of this place
 * she has never met. That difference is the whole point -- it is what turns "there
 * might be more" into a direction to walk in.
 *
 * Bounded deliberately. Only tiles carrying the same name as hers are walked, and
 * only a few of them in each direction, so the answer stays about this place and
 * costs a keypress rather than a pause.
 */
struct zone_report {
    std::string name;
    int tiles_wide = 0;
    int tiles_high = 0;

    int reach_north = 0;
    int reach_east = 0;
    int reach_south = 0;
    int reach_west = 0;

    int seen_north = 0;
    int seen_east = 0;
    int seen_south = 0;
    int seen_west = 0;
};

/** The zone around the player, measured as above. */
zone_report zone_around_player();

} // namespace cata::access
