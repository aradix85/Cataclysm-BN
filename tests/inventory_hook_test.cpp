#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"
#include "inventory_hook.h"
#include "lua_hook_helpers.h"

#include <memory>
#include <string>
#include <vector>

// An inventory_selector needs a player, a map and a curses window, so these
// drive the firing function with the rows the screen reads out of one. What that
// costs is the single call line in src/inventory_ui.cpp; what it buys is that
// every decision about which row the player is told about is asserted here
// rather than only at a keyboard, on the one screen in the game that has no
// message of its own to fall back on.

namespace {

struct seen_inventory {
    int calls = 0;
    std::string title;
    int count = 0;
    sol::optional<int> cursor;
    bool has_entry = false;
    std::string entry_text;
    std::string entry_category;
    std::string entry_where;
    std::string entry_denial;
    bool entry_enabled = false;
    int entry_marked = 0;
};

// Reads one firing of the hook out of the params table.
void record(const std::shared_ptr<seen_inventory>& out, const sol::table& params) {
    ++out->calls;
    out->title = params["title"].get<sol::optional<std::string>>().value_or("");
    out->count = params["count"].get<sol::optional<int>>().value_or(-1);
    out->cursor = params["cursor"].get<sol::optional<int>>();

    const auto entry = params["entry"].get<sol::optional<sol::table>>();
    out->has_entry = entry.has_value();
    if (!entry) { return; }
    out->entry_text = (*entry)["text"].get<std::string>();
    out->entry_category = (*entry)["category"].get<std::string>();
    out->entry_where = (*entry)["where"].get<std::string>();
    out->entry_denial = (*entry)["denial"].get<std::string>();
    out->entry_enabled = (*entry)["enabled"].get<bool>();
    out->entry_marked = (*entry)["marked"].get<int>();
}

cata::inventory_line row(const std::string& text, const std::string& category) {
    cata::inventory_line line;
    line.text = text;
    line.category = category;
    line.where = "character";
    line.enabled = true;
    return line;
}

// A pocket knife, a rock and two cans of beans, with the cursor on the rock.
std::vector<cata::inventory_line> carried() {
    std::vector<cata::inventory_line> lines{
        row("pocket knife", "WEAPONS"),
        row("rock", "TOOLS"),
        row("2 cans of beans", "FOOD"),
    };
    lines[1].selected = true;
    return lines;
}

} // namespace

// An inventory that is open has to arrive whole: what the screen is called, how
// much is in it, and the row the cursor sits on with its own name and heading.
// That is everything the reading model needs to say an overview line on opening
// and then one row per keypress.
TEST_CASE("lua_hook_on_inventory_carries_the_screen_and_its_selected_row", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_inventory>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_inventory", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    cata::fire_on_inventory("Inventory", carried());

    CHECK(seen->calls == 1);
    CHECK(seen->title == "Inventory");
    CHECK(seen->count == 3);
    REQUIRE(seen->cursor.has_value());
    CHECK(*seen->cursor == 2);
    REQUIRE(seen->has_entry);
    CHECK(seen->entry_text == "rock");
    CHECK(seen->entry_category == "TOOLS");
    CHECK(seen->entry_enabled);
    CHECK(seen->entry_marked == 0);
}

// The position is counted among the rows that are handed over, and those are
// the items alone. A screen draws its category headings as rows of their own and
// the cursor steps over them, so counting them would put every position spoken
// out by however many headings sit above the cursor -- and a player who cannot
// see the list has nothing to check that against.
TEST_CASE("lua_hook_on_inventory_counts_positions_among_items_only", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_inventory>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_inventory", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    std::vector<cata::inventory_line> lines = carried();
    lines[2].selected = true;
    lines[1].selected = false;
    cata::fire_on_inventory("Inventory", lines);

    CHECK(seen->count == 3);
    REQUIRE(seen->cursor.has_value());
    CHECK(*seen->cursor == 3);
    CHECK(seen->entry_text == "2 cans of beans");
}

// A screen that cannot be acted on has to say so with its reason. "Wield" that
// does nothing is a dead key to a player without sight, and the reason the game
// draws beside the row is the only thing that separates a rule from a fault.
TEST_CASE("lua_hook_on_inventory_reports_a_row_that_cannot_be_chosen", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_inventory>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_inventory", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    std::vector<cata::inventory_line> lines = carried();
    lines[1].enabled = false;
    lines[1].denial = "Your hands are too small";
    cata::fire_on_inventory("Wield item", lines);

    REQUIRE(seen->has_entry);
    CHECK_FALSE(seen->entry_enabled);
    CHECK(seen->entry_denial == "Your hands are too small");
}

// How many of a stack are marked is the whole state of the drop and pick-up
// screens: marking is what those screens are for, and the mark is drawn rather
// than spoken by the game.
TEST_CASE("lua_hook_on_inventory_reports_how_many_of_a_stack_are_marked", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_inventory>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_inventory", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    std::vector<cata::inventory_line> lines = carried();
    lines[1].marked = 2;
    cata::fire_on_inventory("Drop items", lines);

    REQUIRE(seen->has_entry);
    CHECK(seen->entry_marked == 2);
}

// Where the item is, which the screen answers only by which column it drew the
// row in. A pile at the player's own feet gets the same heading as the pack it
// would go into, so without this a drop screen and a pick-up screen read alike.
TEST_CASE("lua_hook_on_inventory_reports_where_the_item_is", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_inventory>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_inventory", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    std::vector<cata::inventory_line> lines = carried();
    lines[1].where = "map";
    cata::fire_on_inventory("Pick up", lines);

    REQUIRE(seen->has_entry);
    CHECK(seen->entry_where == "map");
}

// An empty screen, or a filter that matched nothing, leaves no selection at all.
// Handing over a zeroed one would have the layer name a row that is not there,
// which is the failure that cannot be noticed without sight.
TEST_CASE("lua_hook_on_inventory_leaves_the_selection_unset_when_there_is_none", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_inventory>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_inventory", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    cata::fire_on_inventory("Inventory", {});

    CHECK(seen->calls == 1);
    CHECK(seen->count == 0);
    CHECK_FALSE(seen->cursor.has_value());
    CHECK_FALSE(seen->has_entry);
}
