#pragma once

#include "coordinates.h"

#include <string>
#include <vector>

namespace cata::access
{

/**
 * Whether the character has any memory of a map square.
 *
 * The game keeps what it has drawn in two forms and fills exactly one of them: the
 * tileset renderer writes a tile name, the text display writes a symbol. Asking
 * about the tile alone answers "never seen" for every square of a game played
 * without a tileset -- which is how this layer forgot a corridor the moment its
 * player walked out of it. The game's own `has_memory_at` asks both, and so does
 * this.
 */
bool remembers_square( const tripoint_abs_ms &p );

/**
 * Whether the character knows a square at all: she can see it now, or she has seen
 * it before and the map remembered it.
 *
 * The filter every report about the world passes through, so that the layer reads
 * the game to her rather than playing it for her. Sight is asked second because it
 * is the expensive half.
 */
bool knows_square( const tripoint_bub_ms &p );

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

/**
 * One way out of the place she is standing in: a door, a staircase, a hole in a
 * wall -- anything the game marks as leading somewhere else.
 *
 * `kind` is `door`, `up` or `down`, which is what decides whether walking to it is
 * worth the crossing. The offset is from her, in squares, because a bearing is what
 * gets spoken and a position on a map is not.
 */
struct zone_exit {
    std::string name;
    std::string kind;
    int dx = 0;
    int dy = 0;
};

/**
 * Every way out she knows of, within the place she is standing in, nearest first.
 *
 * Known is the whole point: what she can see now, plus what the map memory kept
 * from before. A door behind a wall in a room nobody has entered is on the map and
 * is not hers to be told about -- the layer makes the game readable, not easier.
 *
 * Bounded by the loaded map as well as by the zone, since terrain outside it does
 * not exist to be asked about; what she remembers of a place she has walked out of
 * is a different question and is not answered here.
 */
std::vector<zone_exit> exits_in_zone();

/**
 * One kind of place she knows of, with how many of them there are and where the
 * nearest one lies.
 *
 * The offsets are in region tiles, not squares: this is the scale the overmap is
 * read at, and one tile is a walk across a screen.
 */
struct known_place {
    std::string name;
    int count = 0;
    int dx = 0;
    int dy = 0;
};

/**
 * What kinds of place she knows about around her, most nearly first.
 *
 * The inversion of the game's own search, and the reason for it: searching means
 * typing a name, and a name cannot be typed by someone who has never been told it
 * exists. This answers the question the search cannot -- what is out there at all
 * -- from exactly the same stock, since it counts only tiles the overmap marks as
 * seen and looks as far as the search does.
 */
std::vector<known_place> known_places();

} // namespace cata::access
