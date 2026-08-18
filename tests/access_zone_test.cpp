#include "access_zone.h"
#include "avatar.h"
#include "catch/catch.hpp"
#include "coordinates.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"

// What the layer is allowed to report about the world, and it is one question: does
// she know this square at all. Everything answering about doors, stairs, how far a
// room runs and how much of a place she has seen passes through here, so a wrong
// answer here is a wrong answer everywhere at once and in silence.
//
// The half worth pinning is the memory. The game keeps what it has drawn in two
// forms and fills exactly one of them -- a tile name from the tileset renderer, a
// symbol from the text display -- so asking about the tile alone answers "never
// seen" for every square of a game played without a tileset. That is not an exotic
// configuration: it is what a blind player runs, since a tileset costs load time
// and shows her nothing.

namespace {

// A square with the floor between her and it, so sight can never be what makes it
// known. Offset per case, since map memory belongs to the avatar and outlives
// clear_avatar(): a square another case has already written to would pass this one
// for free.
tripoint_bub_ms unseen_square(const int offset) {
    const tripoint_bub_ms here = get_avatar().bub_pos();
    return tripoint_bub_ms(here.x() + offset, here.y() + offset, here.z() - 1);
}

} // namespace

TEST_CASE("access_zone_counts_a_remembered_symbol_as_known", "[lua]") {
    clear_avatar();
    clear_map();

    avatar& you = get_avatar();
    const tripoint_bub_ms below = unseen_square(7);

    // Asserted before anything is memorized, so the case fails loudly rather than
    // passing on what an earlier one left behind.
    REQUIRE_FALSE(cata::access::remembers_square(bub_to_abs(below)));
    REQUIRE_FALSE(cata::access::knows_square(below));

    you.memorize_symbol(bub_to_abs(below), '#');

    CHECK(cata::access::remembers_square(bub_to_abs(below)));
    CHECK(cata::access::knows_square(below));
}

TEST_CASE("access_zone_counts_a_remembered_tile_as_known", "[lua]") {
    clear_avatar();
    clear_map();

    avatar& you = get_avatar();
    const tripoint_bub_ms below = unseen_square(9);

    REQUIRE_FALSE(cata::access::remembers_square(bub_to_abs(below)));

    you.memorize_tile(bub_to_abs(below), "t_floor", 0, 0);

    CHECK(cata::access::remembers_square(bub_to_abs(below)));
    CHECK(cata::access::knows_square(below));
}

// The square she stands on is lit by her standing on it, which is what makes sight
// assertable without arranging any light.
TEST_CASE("access_zone_counts_what_she_can_see_as_known", "[lua]") {
    clear_avatar();
    clear_map();

    const tripoint_bub_ms here = get_avatar().bub_pos();
    get_map().update_visibility_cache(here.z());

    CHECK(cata::access::knows_square(here));
    CHECK_FALSE(cata::access::remembers_square(bub_to_abs(unseen_square(11))));
}
