#include "access_layer.h"
#include "catalua.h"
#include "catalua_hooks.h"
#include "catalua_impl.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"
#include "mod_manager.h"
#include "tts.h"

#include <memory>
#include <vector>

// The layer is not a mod, so nothing in the mod system proves it loaded. This
// is that proof, and it is the only kind available: whether the layer is alive
// on the opening screen cannot be asserted from a test, since no test draws one.
//
// What is asserted here is the part that failed silently as a mod -- scripts
// that run, and hooks that end up registered in the state they were loaded into.

TEST_CASE("access_layer_loads_into_a_state", "[lua]") {
    // The layer speaks as it comes up. Without a sink of our own that reaches
    // NVDA on this machine, and a test that talks out loud is a test nobody runs
    // twice.
    auto recorder = std::make_unique<tts::recording_sink>();
    tts::recording_sink* recorded = recorder.get();
    std::unique_ptr<tts::sink> previous = tts::set(std::move(recorder));

    std::unique_ptr<cata::lua_state, cata::lua_state_deleter> state = cata::make_wrapped_state();
    cata::init_global_state_tables(*state, std::vector<mod_id>{});
    cata::define_hooks(*state);

    // No hooks before, so a pass cannot come from a state that was already set
    // up: this is the check that the check works.
    REQUIRE_FALSE(cata::has_hooks("on_uilist", {.state = state.get()}));

    cata::access::load_into(*state);

    // One per screen the layer answers. A mistake in main.lua registers none of
    // them and is otherwise indistinguishable from silence at the keyboard.
    CHECK(cata::has_hooks("on_uilist", {.state = state.get()}));
    CHECK(cata::has_hooks("on_query_popup", {.state = state.get()}));
    CHECK(cata::has_hooks("on_add_msg", {.state = state.get()}));
    CHECK(cata::has_hooks("on_debugmsg", {.state = state.get()}));
    CHECK(cata::has_hooks("on_action", {.state = state.get()}));

    // It said its loading word on the way up, which is what stands between the
    // player and twenty seconds of silence.
    CHECK_FALSE(recorded->spoken().empty());

    tts::set(std::move(previous));
}

// Which state answers a hook. The case that matters is the world's state during
// the twenty seconds of data loading: it exists from the start of that wait and
// the layer is loaded into it only at the end, so for the whole of it the world
// holds no handler. Anything the game reports meanwhile -- a mod that fails, a
// JSON error, a world naming a mod that is gone -- stops the game on a blocking
// error screen, and if that firing goes to the world's state it is swallowed and
// the screen says nothing. Silence there is indistinguishable from the loading
// the player was told to expect.

TEST_CASE("hook_state_is_the_one_holding_the_handler", "[lua]") {
    auto recorder = std::make_unique<tts::recording_sink>();
    std::unique_ptr<tts::sink> previous = tts::set(std::move(recorder));

    auto empty_state = []() {
        std::unique_ptr<cata::lua_state, cata::lua_state_deleter> s = cata::make_wrapped_state();
        cata::init_global_state_tables(*s, std::vector<mod_id>{});
        cata::define_hooks(*s);
        return s;
    };

    std::unique_ptr<cata::lua_state, cata::lua_state_deleter> world = empty_state();
    std::unique_ptr<cata::lua_state, cata::lua_state_deleter> boot = empty_state();
    cata::access::load_into(*boot);

    // A world still loading: its state is there, its hook tables are there, and
    // nothing of ours is in them yet.
    CHECK(cata::pick_hook_state("on_debugmsg", world.get(), boot.get()) == boot.get());

    // No world at all -- the opening screen, and everything after leaving a world.
    CHECK(cata::pick_hook_state("on_debugmsg", nullptr, boot.get()) == boot.get());

    // Loading has finished. Both hold a handler now, and the world's wins, so no
    // firing is ever answered twice.
    cata::access::load_into(*world);
    CHECK(cata::pick_hook_state("on_debugmsg", world.get(), boot.get()) == world.get());

    // A hook neither of them handles resolves to something rather than nothing,
    // so a caller reading the results table needs no second shape.
    CHECK(cata::pick_hook_state("on_mapgen_postprocess", world.get(), boot.get()) == world.get());
    CHECK(cata::pick_hook_state("on_mapgen_postprocess", nullptr, nullptr) == nullptr);

    tts::set(std::move(previous));
}
