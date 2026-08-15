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
