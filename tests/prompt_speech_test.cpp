#include "catalua_impl.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"

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
