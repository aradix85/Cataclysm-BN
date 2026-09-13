#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"
#include "lua_hook_helpers.h"
#include "morale_hook.h"

#include <memory>
#include <string>
#include <vector>

// The morale screen, which opens on a key in ordinary play and said nothing at all
// until this hook existed. What is asserted here is the mapping from what the
// screen was given to what the layer is handed: the rows in the order the screen
// lists them, each with the figure drawn beside it; the figures below the list,
// which are there only where they apply; and the key the previous round answered,
// which is the only thing that moves the reading position, since the screen itself
// has no selection to move.

namespace {

// A morale point as the mapping asks about one: a name and a share of the whole.
// The screen's own type is private to player_morale, which is the reason that
// mapping is a template and the reason a test can drive it with this.
struct fake_point {
    std::string name;
    double percent = 0.0;

    std::string get_name() const { return name; }
    double get_percent_contribution() const { return percent; }
};

struct seen_row {
    std::string text;
    std::string value;
};

struct seen_screen {
    int calls = 0;
    std::string title;
    std::string total;
    std::string action;
    std::vector<seen_row> rows;
    std::vector<seen_row> summary;
};

std::vector<seen_row> read_rows(const sol::table& params, const char* key) {
    std::vector<seen_row> out;
    const sol::optional<sol::table> list = params[key];
    if (!list) { return out; }
    for (std::size_t i = 1; i <= list->size(); ++i) {
        const sol::table row = (*list)[i];
        out.push_back(seen_row{row["text"].get<std::string>(), row["value"].get<std::string>()});
    }
    return out;
}

void record(const std::shared_ptr<seen_screen>& out, const sol::table& params) {
    ++out->calls;
    out->title = params["title"].get<sol::optional<std::string>>().value_or("");
    out->total = params["total"].get<sol::optional<std::string>>().value_or("");
    out->action = params["action"].get<sol::optional<std::string>>().value_or("");
    out->rows = read_rows(params, "rows");
    out->summary = read_rows(params, "summary");
}

} // namespace

TEST_CASE("lua_hook_on_morale_hands_over_the_whole_list", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_screen>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_morale", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    const std::vector<fake_point> positive{{"Enjoyed a hot meal", 70.0}, {"Music", 30.0}};
    const std::vector<fake_point> negative{{"Wet", 100.0}};
    const cata::morale_summary at{20, 8, 12, 0, 0, 100};

    // An empty action is the screen opening, which is the firing that has to speak
    // in full.
    cata::fire_on_morale(positive, negative, at, "");

    REQUIRE(seen->calls == 1);
    CHECK(seen->title == "Morale");
    CHECK(seen->action.empty());
    // The figure the whole screen is about, signed as the screen signs it.
    CHECK(seen->total == "+12");

    // Each group's total first and the sources under it, in the order the screen
    // sorted them. A group total is signed; a source is a share of the whole.
    REQUIRE(seen->rows.size() == 5);
    CHECK(seen->rows[0].text == "Total positive morale");
    CHECK(seen->rows[0].value == "+20");
    CHECK(seen->rows[1].text == "Enjoyed a hot meal");
    CHECK(seen->rows[1].value == "70%");
    CHECK(seen->rows[2].text == "Music");
    CHECK(seen->rows[2].value == "30%");
    // The magnitude the game reports, drawn negated, as the screen draws it.
    CHECK(seen->rows[3].text == "Total negative morale");
    CHECK(seen->rows[3].value == "-8");
    CHECK(seen->rows[4].text == "Wet");
    CHECK(seen->rows[4].value == "100%");

    // Focus is always drawn; pain and the fatigue cap are not, and this character
    // has neither. The label loses the colon the screen ends it with, which is
    // punctuation for a column of figures and a word read out loud.
    REQUIRE(seen->summary.size() == 1);
    CHECK(seen->summary[0].text == "Focus trends towards");
    CHECK(seen->summary[0].value == "100");
}

TEST_CASE("lua_hook_on_morale_carries_the_key_that_was_answered", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_screen>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_morale", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    const std::vector<fake_point> positive{{"Music", 100.0}};
    const std::vector<fake_point> negative;

    // The screen does nothing with an arrow key on a list that fits, so this is
    // the only sign the layer has that the reading should move.
    cata::fire_on_morale(positive, negative, cata::morale_summary{5, 0, 5, 0, 0, 100}, "DOWN");

    REQUIRE(seen->calls == 1);
    CHECK(seen->action == "DOWN");
}

TEST_CASE("lua_hook_on_morale_has_no_rows_when_nothing_affects_morale", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_screen>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_morale", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    const std::vector<fake_point> none;
    // A character in pain, with fatigue capping what morale can reach: both are
    // drawn only where they apply, and both apply here.
    cata::fire_on_morale(none, none, cata::morale_summary{0, 0, 0, 8, 50, 90}, "");

    REQUIRE(seen->calls == 1);
    // No group totals either. The screen answers this case with a sentence of its
    // own instead of a list, and a list of no rows is what says so here.
    CHECK(seen->rows.empty());
    CHECK(seen->total == "0");

    REQUIRE(seen->summary.size() == 3);
    CHECK(seen->summary[0].text == "Pain level");
    CHECK(seen->summary[0].value == "-8");
    CHECK(seen->summary[1].text == "Fatigue Morale Cap");
    CHECK(seen->summary[1].value == "50");
    CHECK(seen->summary[2].text == "Focus trends towards");
    CHECK(seen->summary[2].value == "90");
}
