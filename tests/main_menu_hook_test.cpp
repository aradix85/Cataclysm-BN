#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"
#include "lua_hook_helpers.h"
#include "main_menu_hook.h"

#include <memory>
#include <string>
#include <vector>

// The opening screen builds its own input context and draws itself, so nothing
// about it can be reached the way a uilist is. These drive the firing function
// with the fields the screen hands it, which leaves only the one call line in
// src/main_menu.cpp uncovered and puts every decision about what is selected,
// and what hangs under it, under test.

namespace {

struct seen_screen {
    int calls = 0;
    std::string category;
    bool has_heading = false;
    std::string heading_text;
    std::string heading_key;
    int heading_cursor = 0;
    int heading_count = 0;
    bool has_entry = false;
    std::string entry_text;
    std::string entry_key;
    int entry_cursor = 0;
    int entry_count = 0;
};

// Reads one firing of the hook out of the params table.
void record(const std::shared_ptr<seen_screen>& out, const sol::table& params) {
    ++out->calls;
    out->category = params["category"].get<sol::optional<std::string>>().value_or("");

    const auto heading = params["heading"].get<sol::optional<sol::table>>();
    out->has_heading = heading.has_value();
    if (heading) {
        out->heading_text = (*heading)["text"].get<std::string>();
        out->heading_key = (*heading)["key"].get<std::string>();
        out->heading_cursor = (*heading)["cursor"].get<int>();
        out->heading_count = (*heading)["count"].get<int>();
    }

    const auto entry = params["entry"].get<sol::optional<sol::table>>();
    out->has_entry = entry.has_value();
    if (!entry) { return; }
    out->entry_text = (*entry)["text"].get<std::string>();
    out->entry_key = (*entry)["key"].get<std::string>();
    out->entry_cursor = (*entry)["cursor"].get<int>();
    out->entry_count = (*entry)["count"].get<int>();
}

// The headings, as init_strings() writes them: each carries the letter that
// selects it as markup around the character the game highlights.
std::vector<std::string> headings() {
    return {"<M|m>OTD",     "<N|n>ew Game", "Lo<a|A>d",    "<W|w>orld",
            "Se<t|T>tings", "H<e|E|?>lp",   "<C|c>redits", "<Q|q>uit"};
}

std::vector<std::string> new_game_items() {
    return {"C<u|U>stom Character", "<P|p>reset Character", "<R|r>andom Character",
            "Play N<o|O>w!"};
}

std::vector<std::string> settings_items() {
    return {"<O|o>ptions", "Ke<y|Y>bindings", "Colo<r|R>s"};
}

} // namespace

// The screen has to arrive as the two lists it is: which heading of how many is
// selected along the top, and which entry of how many under it. Both are what
// the reading model needs to say where the cursor is without naming the screen
// again on every keypress.
TEST_CASE("lua_hook_on_main_menu_carries_both_levels_of_the_screen", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_screen>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_main_menu", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    cata::fire_on_main_menu(headings(), 1, new_game_items(), settings_items(), 3);

    CHECK(seen->calls == 1);
    CHECK(seen->category == "MAIN_MENU");
    REQUIRE(seen->has_heading);
    CHECK(seen->heading_cursor == 2);
    CHECK(seen->heading_count == 8);
    REQUIRE(seen->has_entry);
    CHECK(seen->entry_cursor == 4);
    CHECK(seen->entry_count == 4);
}

// The letter that selects a heading is drawn as a highlight and written as
// markup. Handed over unresolved it reaches the synthesiser as punctuation --
// "less than N pipe n greater than ew Game" -- so the name that arrives has to
// be the name on screen and nothing else.
TEST_CASE("lua_hook_on_main_menu_says_the_name_and_not_its_hotkey_markup", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_screen>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_main_menu", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    cata::fire_on_main_menu(headings(), 1, new_game_items(), settings_items(), 0);
    CHECK(seen->heading_text == "New Game");
    CHECK(seen->entry_text == "Custom Character");

    // The key comes over beside the name rather than inside it, so a handler can
    // say the name alone, or say both, without parsing anything back apart. It
    // is the case the game highlights, which is not always the lower one.
    CHECK(seen->heading_key == "N");
    CHECK(seen->entry_key == "u");

    // A heading whose letter is not the first one, and a list of its own.
    cata::fire_on_main_menu(headings(), 4, new_game_items(), settings_items(), 1);
    CHECK(seen->heading_text == "Settings");
    CHECK(seen->heading_key == "t");
    CHECK(seen->entry_text == "Keybindings");
    CHECK(seen->entry_key == "y");

    // Three letters select this one. The first is the one the game highlights,
    // and saying all three would be reading out punctuation.
    cata::fire_on_main_menu(headings(), 5, new_game_items(), settings_items(), 0);
    CHECK(seen->heading_text == "Help");
    CHECK(seen->heading_key == "e");

    // A name the game marks no key on keeps its name and gets no key. World
    // names are the real case: they are reached with the arrow keys only.
    cata::fire_on_main_menu(headings(), 4, new_game_items(), {"Boston (2)"}, 0);
    CHECK(seen->entry_text == "Boston (2)");
    CHECK(seen->entry_key.empty());
}

// Help and Quit carry no list at all, and MOTD and Credits show a page of text
// rather than one. Left unset rather than zeroed, a handler reads "nothing under
// this heading" instead of announcing whichever entry a zero would point at.
TEST_CASE("lua_hook_on_main_menu_has_no_entry_where_the_heading_carries_no_list", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_screen>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_main_menu", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    cata::fire_on_main_menu(headings(), 5, new_game_items(), settings_items(), 0);
    CHECK(seen->calls == 1);
    REQUIRE(seen->has_heading);
    CHECK_FALSE(seen->has_entry);

    // The same answer for a selection left pointing past the end of a shorter
    // list, which is what switching between headings of different lengths does.
    cata::fire_on_main_menu(headings(), 4, new_game_items(), settings_items(), 6);
    CHECK(seen->calls == 2);
    REQUIRE(seen->has_heading);
    CHECK_FALSE(seen->has_entry);
}

// A selection outside the row of headings describes no screen, and a firing
// carrying no heading would leave a handler with nothing to say about a keypress
// that did something. Nothing is handed over at all instead.
TEST_CASE("lua_hook_on_main_menu_does_not_fire_without_a_heading", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_screen>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_main_menu", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    cata::fire_on_main_menu(headings(), 8, new_game_items(), settings_items(), 0);
    cata::fire_on_main_menu(headings(), -1, new_game_items(), settings_items(), 0);
    cata::fire_on_main_menu({}, 0, new_game_items(), settings_items(), 0);

    CHECK(seen->calls == 0);
}

// The hook has to exist as a table before anything can insert into it: a name
// missing from `hook_names` makes every registration fail without a word. And
// since this fires on every keypress the screen sees, an unregistered hook must
// build no tables at all.
TEST_CASE("lua_hook_on_main_menu_is_declared_and_idle_when_nothing_listens", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto declared = lua["game"]["hooks"]["on_main_menu"].get<sol::optional<sol::table>>();
    CHECK(declared.has_value());

    const test_lua_hooks::emptied_hook empty{lua, "on_main_menu"};
    REQUIRE_FALSE(cata::has_hooks("on_main_menu"));

    cata::fire_on_main_menu(headings(), 0, new_game_items(), settings_items(), 0);
}
