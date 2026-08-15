#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"
#include "lua_hook_helpers.h"
#include "ui.h"
#include "ui_hook.h"

#include <memory>
#include <string>
#include <vector>

// A uilist refuses to be constructed in test mode, so these drive the firing
// function with the fields the menu hands it. What that costs is the single call
// line in src/ui.cpp; what it buys is that every decision the hook makes about
// which entry the cursor is on is asserted here rather than only at a keyboard.

namespace {

struct seen_menu {
    int calls = 0;
    std::string category;
    std::string title;
    std::string text;
    int count = 0;
    sol::optional<int> cursor;
    bool has_entry = false;
    std::string entry_text;
    std::string entry_desc;
    std::string entry_column;
    bool entry_enabled = false;
};

// Reads one firing of the hook out of the params table.
void record(const std::shared_ptr<seen_menu>& out, const sol::table& params) {
    ++out->calls;
    out->category = params["category"].get<sol::optional<std::string>>().value_or("");
    out->title = params["title"].get<sol::optional<std::string>>().value_or("");
    out->text = params["text"].get<sol::optional<std::string>>().value_or("");
    out->count = params["count"].get<sol::optional<int>>().value_or(-1);
    out->cursor = params["cursor"].get<sol::optional<int>>();

    const auto entry = params["entry"].get<sol::optional<sol::table>>();
    out->has_entry = entry.has_value();
    if (!entry) { return; }
    out->entry_text = (*entry)["text"].get<std::string>();
    out->entry_desc = (*entry)["desc"].get<std::string>();
    out->entry_column = (*entry)["column"].get<std::string>();
    out->entry_enabled = (*entry)["enabled"].get<bool>();
}

// The item action menu, as the game builds it.
std::vector<uilist_entry> item_actions() {
    return {
        {0, true, MENU_AUTOASSIGN, "Wield", "Hold it in your hands", "w"},
        {1, true, MENU_AUTOASSIGN, "Reload", "Put ammunition into it", "r"},
        {2, true, MENU_AUTOASSIGN, "Drop", "Put it on the ground", "d"},
    };
}

} // namespace

// A menu that is open has to arrive whole: what it is called, how much is in it,
// and the entry the cursor sits on with its own text, description and second
// column. That is everything the reading model needs to say an overview line on
// opening and then one entry per keypress.
TEST_CASE("lua_hook_on_uilist_carries_the_menu_and_its_selected_entry", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_menu>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_uilist", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    cata::fire_on_uilist(
        "UILIST", "Item actions", "What do you want to do?", item_actions(), {0, 1, 2}, 1);

    CHECK(seen->calls == 1);
    CHECK(seen->category == "UILIST");
    CHECK(seen->title == "Item actions");
    CHECK(seen->text == "What do you want to do?");
    CHECK(seen->count == 3);
    REQUIRE(seen->cursor.has_value());
    CHECK(*seen->cursor == 2);
    REQUIRE(seen->has_entry);
    CHECK(seen->entry_text == "Reload");
    CHECK(seen->entry_desc == "Put ammunition into it");
    CHECK(seen->entry_column == "r");
}

// The menu's cursor counts positions on screen, not entries in the list, and the
// two only agree while nothing is filtered out. Announcing the wrong entry is
// silent to a player who cannot see the highlight, so this is the assertion that
// has to fail loudly if the two indices are ever confused.
TEST_CASE("lua_hook_on_uilist_cursor_counts_what_the_filter_left", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_menu>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_uilist", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    // A filter matching "r" leaves the second and third entries visible; the
    // cursor is on the second of those two, which is the third entry.
    cata::fire_on_uilist("UILIST", "Item actions", "", item_actions(), {1, 2}, 1);

    CHECK(seen->count == 2);
    REQUIRE(seen->cursor.has_value());
    CHECK(*seen->cursor == 2);
    REQUIRE(seen->has_entry);
    CHECK(seen->entry_text == "Drop");
}

// A menu shows what cannot be chosen greyed out. Without that flag a player is
// told an option exists, presses it, and nothing happens with no explanation.
TEST_CASE("lua_hook_on_uilist_reports_an_entry_that_cannot_be_chosen", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_menu>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_uilist", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    const std::vector<uilist_entry> entries{
        {0, true, MENU_AUTOASSIGN, "Wield", "", ""},
        {1, false, MENU_AUTOASSIGN, "Reload", "You have no ammunition for it", ""},
    };

    cata::fire_on_uilist("UILIST", "Item actions", "", entries, {0, 1}, 0);
    REQUIRE(seen->has_entry);
    CHECK(seen->entry_enabled);

    cata::fire_on_uilist("UILIST", "Item actions", "", entries, {0, 1}, 1);
    REQUIRE(seen->has_entry);
    CHECK(seen->entry_text == "Reload");
    CHECK_FALSE(seen->entry_enabled);
}

// Typing a filter that matches nothing leaves the menu open with an empty list,
// and the menu answers -1 for its cursor. Leaving both params unset rather than
// zeroed means a handler reads "no selection" instead of announcing whichever
// entry a zero would have pointed at.
TEST_CASE("lua_hook_on_uilist_has_no_cursor_when_nothing_is_selectable", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_menu>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_uilist", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    cata::fire_on_uilist("UILIST", "Item actions", "", item_actions(), {}, -1);

    REQUIRE(seen->calls == 1);
    CHECK(seen->count == 0);
    CHECK_FALSE(seen->cursor.has_value());
    CHECK_FALSE(seen->has_entry);

    // Same answer for a cursor left pointing past the end of the filtered list.
    cata::fire_on_uilist("UILIST", "Item actions", "", item_actions(), {0}, 3);

    CHECK(seen->calls == 2);
    CHECK_FALSE(seen->cursor.has_value());
    CHECK_FALSE(seen->has_entry);
}

// The hook has to exist as a table before any mod can insert into it: a name
// missing from `hook_names` makes every registration fail without a word. And
// since every menu in the game fires this, an unregistered hook must build no
// tables at all.
TEST_CASE("lua_hook_on_uilist_is_declared_and_idle_when_nothing_listens", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto declared = lua["game"]["hooks"]["on_uilist"].get<sol::optional<sol::table>>();
    CHECK(declared.has_value());

    const test_lua_hooks::emptied_hook empty{lua, "on_uilist"};
    REQUIRE_FALSE(cata::has_hooks("on_uilist"));

    cata::fire_on_uilist("UILIST", "Nobody is listening", "", item_actions(), {0, 1, 2}, 0);
}
