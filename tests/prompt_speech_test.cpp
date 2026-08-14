#include "catalua_impl.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"
#include "map_helpers.h"
#include "player_helpers.h"

#include <string>

// The prompt-speech decisions live in data/mods/bn_access/lib/prompts.lua and
// are pure: tables in, list of strings out. That is what makes them assertable
// here, with no game running and no NVDA present.
//
// The script is driven exactly as the mod loads the module, so a mistake in the
// module is a failure here rather than a silence the owner has to diagnose by
// ear in a live game.

namespace {

std::string result_of(sol::state& lua, const char* key) {
    return lua.globals()["test_data"][key].get<std::string>();
}

} // namespace

TEST_CASE("bn_access_prompt_utterances", "[lua]") {
    sol::state lua = make_lua_state();
    sol::table test_data = lua.create_table();
    lua.globals()["test_data"] = test_data;

    run_lua_script(lua, "tests/lua/prompt_utterances_test.lua");

    // One utterance, one subject (F1): the question is never interleaved with
    // the options, and each option stands alone. Name first (P2), so the player
    // can stop listening as soon as they know enough. The doubled space in the
    // game's own text is collapsed.
    CHECK(result_of(lua, "first_showing") == "You may be attacked! Proceed? / Yes, selected / No");

    // The hook fires once per input round, so an unanswered prompt arrives
    // again after every keypress. Saying nothing is what stops it talking over
    // itself (P5).
    CHECK(result_of(lua, "unchanged").empty());

    // F2: a selection that moves without being announced is the failure that
    // cost a blind CDDA player playability. Only what changed is spoken.
    CHECK(result_of(lua, "cursor_moved") == "No, selected");

    // Markup and the waiting bar are drawn, not said (issue #55436: a screen
    // reader given ASCII decoration reads out the glyphs).
    CHECK(result_of(lua, "waiting") == "Please wait\xE2\x80\xA6");

    // The bar cycles through | / - \ on every redraw. Comparing on the raw text
    // would make every frame a new prompt and never stop talking.
    CHECK(result_of(lua, "waiting_spun").empty());

    // Nothing is ever replaced by a count. Measured across src/: the largest
    // query_popup in the game offers four options, so a prompt is bounded by
    // the game itself, and summarising one would hide what has to be answered.
    CHECK(result_of(lua, "largest_prompt")
          == "Stop and drop the plank? / Yes / No / Activity manager, selected / "
             "Ignore further distractions");

    // Silence is information (P5): no text and no options says nothing at all.
    CHECK(result_of(lua, "empty").empty());
}

TEST_CASE("bn_access_message_policy", "[lua]") {
    sol::state lua = make_lua_state();
    sol::table test_data = lua.create_table();
    lua.globals()["test_data"] = test_data;

    run_lua_script(lua, "tests/lua/message_speech_test.lua");

    // Ordinary prose is spoken; a line of drawing characters is not (issue
    // #55436), and debug messages belong to a developer with a console open.
    CHECK(result_of(lua, "speaks_prose") == "true");
    CHECK(result_of(lua, "speaks_border") == "false");
    CHECK(result_of(lua, "speaks_debug") == "false");

    // Speech queues at NVDA and nothing in the game throttles it (P8), so bad
    // news has to be able to pass ahead of what is already waiting (P7). Never
    // "now": that discards what is speaking, and in a fight every hit is bad
    // news, so each would silence the one before it.
    CHECK(result_of(lua, "urgency_bad") == "next");
    CHECK(result_of(lua, "urgency_warning") == "next");
    CHECK(result_of(lua, "urgency_info") == "normal");
    CHECK(result_of(lua, "urgency_good") == "normal");

    // Markup and the whitespace left by terminal wrapping are drawing, not
    // speech, and reach the layer inside otherwise ordinary prose.
    CHECK(result_of(lua, "cleaned") == "You are bleeding badly.");
}

// Bearings are the one thing a player cannot check for themselves: a wrong
// direction sends them into a wall or into a zombie, and nothing in the game
// would contradict it. So every compass point is pinned here.
TEST_CASE("bn_access_surroundings_overview", "[lua]") {
    sol::state lua = make_lua_state();
    sol::table test_data = lua.create_table();
    lua.globals()["test_data"] = test_data;

    run_lua_script(lua, "tests/lua/surroundings_test.lua");

    CHECK(result_of(lua, "east") == "east");
    CHECK(result_of(lua, "northeast") == "northeast");
    CHECK(result_of(lua, "north") == "north");
    CHECK(result_of(lua, "northwest") == "northwest");
    CHECK(result_of(lua, "west") == "west");
    CHECK(result_of(lua, "southwest") == "southwest");
    CHECK(result_of(lua, "south") == "south");
    CHECK(result_of(lua, "southeast") == "southeast");

    // Ten east and one south is east. Snapping by the sign of each axis would
    // call it southeast and make a long approach wobble between two words.
    CHECK(result_of(lua, "mostly_east") == "east");

    // Standing on the thing has no direction, and the caller has to be able to
    // tell that apart from a direction.
    CHECK(result_of(lua, "zero") == "nil");

    // A diagonal is one step, as it is when walking, so the number spoken is the
    // number of moves rather than a distance the player has to convert.
    CHECK(test_data["diagonal_distance"].get<int>() == 3);
    CHECK(result_of(lua, "described") == "4 northeast");

    // Grouped, enemies first, one utterance per group, nearest named (P2, P4).
    CHECK(
        result_of(lua, "overview")
        == "3 enemies. Nearest: skeleton, 2 southeast. / "
           "1 creature. rabbit, 6 southeast. / "
           "1 way out. closed wood door, 3 east.");

    // One reads as one rather than as a count with a nearest attached.
    CHECK(result_of(lua, "single") == "1 enemy. zombie, 2 north.");

    // Asked and answered (P5 is about not volunteering emptiness, not about
    // ignoring a question).
    CHECK(result_of(lua, "empty") == "Nothing nearby.");

    // A refused move is the one case the game itself never reports: it assumes
    // the wall was seen. Without this, a direction key produces nothing at all
    // and cannot be told apart from a key that never arrived.
    CHECK(result_of(lua, "blocked") == "wall, blocked.");
    CHECK(result_of(lua, "blocked_unchanged") == "wall, blocked.");

    // A step onto different ground names it; a step across the same floor says
    // nothing, or every step would bury the blocked case.
    CHECK(result_of(lua, "changed") == "grass.");
    CHECK(result_of(lua, "unchanged").empty());
}

// The perception queries are the layer's eyes. A binding that is missing or
// misnamed does not crash: the collector simply never reports that channel, and
// the player is never told the thing exists. That is the infrared-goggles bug,
// so the surface is asserted here rather than trusted.
TEST_CASE("bn_access_perception_bindings", "[lua]") {
    clear_avatar();
    clear_map();

    sol::state lua = make_lua_state();
    sol::table test_data = lua.create_table();
    lua.globals()["test_data"] = test_data;

    run_lua_script(lua, "tests/lua/perception_bindings_test.lua");

    // The avatar is standing there, so the square is passable and has a floor.
    // That holds on any map, which is what makes it assertable at all.
    CHECK(test_data["standing_is_passable"].get<std::string>() == "true");
    CHECK(test_data["has_floor"].get<std::string>() == "true");
    CHECK(test_data["move_cost"].get<int>() > 0);

    // Percentages, not the one-in-a-million the game stores them as.
    CHECK(test_data["coverage"].get<int>() >= 0);
    CHECK(test_data["coverage"].get<int>() <= 100);
    CHECK(test_data["block_chance"].get<int>() >= 0);
    CHECK(test_data["block_chance"].get<int>() <= 100);

    // Terrain always describes itself; furniture only when there is any, and an
    // empty string rather than a nil is what lets a caller treat both alike.
    CHECK_FALSE(test_data["ter_description"].get<std::string>().empty());
    CHECK(test_data["furn_description"].get<sol::optional<std::string>>().has_value());
    CHECK(test_data["signage"].get<sol::optional<std::string>>().has_value());
    CHECK(test_data["sound"].get<sol::optional<std::string>>().has_value());
    CHECK(test_data["footsteps"].get<int>() >= 0);

    // The special senses reach a creature the eyes cannot. Nothing is asserted
    // about the answer for the avatar itself -- only that both questions can be
    // asked, and that both descriptions come back as a list.
    CHECK(test_data["infrared_self"].get<sol::optional<std::string>>().has_value());
    CHECK(test_data["specials_self"].get<sol::optional<std::string>>().has_value());
    CHECK(test_data["describe_infrared"].get<int>() >= 0);
    CHECK(test_data["describe_specials"].get<int>() >= 0);
}

// A syntax error anywhere in the mod's own scripts is invisible until the game
// loads it: the mod then registers nothing, announces nothing, and looks
// exactly like a broken build to a player who cannot see the error. Parsing
// them here turns that into a red test instead of a wasted session at the
// keyboard. The scripts are only loaded, never run -- running them needs a game.
TEST_CASE("bn_access_scripts_parse", "[lua]") {
    sol::state lua = make_lua_state();

    for (const char* script : {
             "data/mods/bn_access/main.lua",
             "data/mods/bn_access/lib/speech.lua",
             "data/mods/bn_access/lib/text.lua",
             "data/mods/bn_access/lib/messages.lua",
             "data/mods/bn_access/lib/bearing.lua",
             "data/mods/bn_access/lib/movement.lua",
             "data/mods/bn_access/lib/surroundings.lua",
             "data/mods/bn_access/lib/prompts.lua",
         }) {
        INFO(script);
        sol::load_result loaded = lua.load_file(script);
        const bool valid = loaded.valid();
        if (!valid) {
            const sol::error err = loaded;
            INFO(err.what());
            CHECK(valid);
        } else {
            CHECK(valid);
        }
    }
}
