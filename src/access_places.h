#pragma once

#include "coordinates.h"

#include <string>

namespace cata::access
{

/**
 * The name of the region an overmap tile belongs to -- "overgrown cabin",
 * "forest" -- which is what the game's own look-around window prints above a
 * square, and the shorter of the two ways it names a place.
 *
 * Short on purpose: this is the one that can be said while walking, and a
 * sentence with a city in it read out at every field boundary would be the
 * flood F5 warns about.
 */
std::string area_name_at( const tripoint_abs_omt &p );

/**
 * The same, for a square of the loaded map, which is the shape every other
 * perception query takes and the shape the layer has in hand: a step's
 * destination, a look-around cursor, the player's own feet. The projection up to
 * overmap scale is done here rather than in script, because script has no way to
 * hold a coordinate of that scale that the bindings will accept back.
 */
std::string area_name_at( const tripoint_bub_ms &p );

/**
 * The same place as the overmap's own sidebar describes it -- "house in central
 * Springfield" -- which is the region name with the nearest known city and the
 * direction from it, or the bare name where no city is known or the tile is off
 * the ground level.
 *
 * The region's display terrain stands in where the tile is the region's own
 * default and the region names one, exactly as the sidebar chooses between the
 * two. That branch is the one piece of the screen's reasoning the layer
 * reproduces rather than reading a value off it, and it lives here so that the
 * overmap's firing point and script cannot drift apart about the same tile.
 */
std::string place_description_at( const tripoint_abs_omt &p );

} // namespace cata::access
