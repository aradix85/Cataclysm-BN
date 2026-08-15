#include "catalua_impl.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"
#include "tts.h"

#include <memory>
#include <string>

// Tests for the speech bridge. Everything here runs headless: no NVDA, no DLL,
// no Windows required. What is deliberately NOT tested is that NVDA spoke --
// that is not checkable without a person listening. What is tested is that the
// right string, on the right channel, at the right priority, reached the sink.

namespace {

// Installs a recording sink for the duration of a test and puts the previous
// sink back afterwards, so one test cannot leave the sink swapped for the next.
class scoped_recorder {
public:
    scoped_recorder() {
        auto owned = std::make_unique<tts::recording_sink>();
        rec = owned.get();
        previous = tts::set(std::move(owned));
    }
    ~scoped_recorder() { tts::set(std::move(previous)); }

    tts::recording_sink* rec = nullptr;

private:
    std::unique_ptr<tts::sink> previous;
};

} // namespace

TEST_CASE("tts_escape_ssml", "[tts]") {
    // Plain text must survive untouched, or every ordinary utterance would be
    // mangled on its way to NVDA.
    CHECK(tts::escape_ssml("three zombies, four northeast") == "three zombies, four northeast");

    CHECK(tts::escape_ssml("").empty());

    // The five XML metacharacters.
    CHECK(tts::escape_ssml("<") == "&lt;");
    CHECK(tts::escape_ssml(">") == "&gt;");
    CHECK(tts::escape_ssml("\"") == "&quot;");
    CHECK(tts::escape_ssml("'") == "&apos;");
    CHECK(tts::escape_ssml("&") == "&amp;");

    // The ampersand has to be escaped in the same pass as the rest, not before
    // or after it: escaping in two passes turns "<" into "&amp;lt;" and the
    // player hears the entity read out.
    CHECK(tts::escape_ssml("a<b") == "a&lt;b");
    CHECK(tts::escape_ssml("&lt;") == "&amp;lt;");
    CHECK(tts::escape_ssml("&&") == "&amp;&amp;");

    // Non-ASCII is not an XML metacharacter and must pass through as bytes;
    // the conversion to UTF-16 happens later, in the sink.
    CHECK(tts::escape_ssml("vier noordoost \xE2\x80\x94 zombie")
          == "vier noordoost \xE2\x80\x94 zombie");
}

TEST_CASE("tts_recording_sink_captures_what_was_said", "[tts]") {
    scoped_recorder r;

    tts::get().speak("zombie, four northeast", tts::priority::normal);
    tts::get().braille("zombie 4 NE");
    tts::get().cancel_speech();

    REQUIRE(r.rec->spoken().size() == 1);
    CHECK(r.rec->spoken()[0].text == "zombie, four northeast");
    CHECK(r.rec->spoken()[0].prio == tts::priority::normal);
    REQUIRE(r.rec->brailled().size() == 1);
    CHECK(r.rec->brailled()[0] == "zombie 4 NE");
    CHECK(r.rec->cancels() == 1);

    r.rec->clear();
    CHECK(r.rec->spoken().empty());
    CHECK(r.rec->brailled().empty());
    CHECK(r.rec->cancels() == 0);
}

TEST_CASE("tts_output_reaches_both_channels", "[tts]") {
    scoped_recorder r;

    // The one-string form is the ordinary case: speech and braille get the same
    // text. This is what keeps braille from being forgotten at a call site.
    tts::output("you are bleeding");

    REQUIRE(r.rec->spoken().size() == 1);
    REQUIRE(r.rec->brailled().size() == 1);
    CHECK(r.rec->spoken()[0].text == "you are bleeding");
    CHECK(r.rec->brailled()[0] == "you are bleeding");
    CHECK(r.rec->spoken()[0].prio == tts::priority::normal);
}

TEST_CASE("tts_output_keeps_the_two_forms_apart", "[tts]") {
    scoped_recorder r;

    // Whether any utterance should read differently in braille than in speech
    // is an open question the owner settles by reading. This asserts only that
    // the layer can carry two forms without one leaking into the other.
    tts::output("four northeast", "4 NE", tts::priority::next);

    REQUIRE(r.rec->spoken().size() == 1);
    REQUIRE(r.rec->brailled().size() == 1);
    CHECK(r.rec->spoken()[0].text == "four northeast");
    CHECK(r.rec->brailled()[0] == "4 NE");
    CHECK(r.rec->spoken()[0].prio == tts::priority::next);
}

TEST_CASE("tts_sink_swap_restores_the_previous_sink", "[tts]") {
    tts::sink& before = tts::get();

    {
        scoped_recorder r;
        CHECK(&tts::get() != &before);
        CHECK(tts::get().is_available());
    }

    // A test that leaves the recording sink installed would silence the game
    // for every test after it, and nothing would report that.
    CHECK(&tts::get() == &before);
}

TEST_CASE("tts_default_sink_is_safe_without_nvda", "[tts]") {
    // The default sink resolves nvdaControllerClient.dll at runtime. In the test
    // binary the DLL is normally absent, and on non-Windows there is no DLL at
    // all -- neither may be an error. The requirement is that the layer stays
    // silent instead of throwing, so a build without NVDA still runs the game.
    //
    // is_available() is deliberately not asserted either way: it is true when a
    // developer happens to have the DLL beside the binary and NVDA running, and
    // a test that fails in that case would be testing the machine, not the code.
    tts::reset();

    CHECK_NOTHROW(tts::get().speak("still here", tts::priority::normal));
    CHECK_NOTHROW(tts::get().speak("urgent", tts::priority::now));
    CHECK_NOTHROW(tts::get().braille("still here"));
    CHECK_NOTHROW(tts::get().cancel_speech());
    CHECK_NOTHROW(tts::get().is_available());
    CHECK_NOTHROW(tts::output("both channels"));
}

TEST_CASE("tts_lua_speech_reaches_the_sink", "[lua][tts]") {
    scoped_recorder r;

    sol::state lua = make_lua_state();
    run_lua_script(lua, "tests/lua/tts_speech_test.lua");

    // Speech channel: say(), speech-only, and the split call. The braille-only
    // call must not appear here.
    REQUIRE(r.rec->spoken().size() == 3);
    CHECK(r.rec->spoken()[0].text == "three zombies");
    CHECK(r.rec->spoken()[0].prio == tts::priority::normal);
    CHECK(r.rec->spoken()[1].text == "spoken only");
    CHECK(r.rec->spoken()[2].text == "four northeast");
    CHECK(r.rec->spoken()[2].prio == tts::priority::next);

    // Braille channel: say() reaches it with the same text, braille-only with
    // its own, and the split call with the braille form. Speech-only must not
    // appear -- that is the whole point of the "_only" names.
    REQUIRE(r.rec->brailled().size() == 3);
    CHECK(r.rec->brailled()[0] == "three zombies");
    CHECK(r.rec->brailled()[1] == "brailled only");
    CHECK(r.rec->brailled()[2] == "4 NE");

    CHECK(r.rec->cancels() == 1);
}

TEST_CASE("tts_priority_from_int_rejects_nonsense", "[tts]") {
    CHECK(tts::priority_from_int(0) == tts::priority::normal);
    CHECK(tts::priority_from_int(1) == tts::priority::next);
    CHECK(tts::priority_from_int(2) == tts::priority::now);

    // A script can pass anything. Falling back to normal keeps the utterance
    // audible; silently dropping it would leave the player with no way to tell
    // that something had been said at all.
    CHECK(tts::priority_from_int(-1) == tts::priority::normal);
    CHECK(tts::priority_from_int(3) == tts::priority::normal);
    CHECK(tts::priority_from_int(99999) == tts::priority::normal);
}

// The suite runs the game, and the game says things: an upstream test that eats
// while full raises a real prompt, and the layer is loaded into the world the
// suite builds, so it answers that prompt the way it answers a player's. With
// the platform sink as the default, the machine running the tests is spoken to
// and brailled at by a game nobody is playing.
//
// So the default destination in a test binary is a recorder. This asserts what
// `reset()` restores, which is what any test that never installs a sink of its
// own gets -- including every upstream test, which knows nothing about any of
// this and never will.
TEST_CASE("tts_default_sink_in_a_test_binary_speaks_to_nobody", "[tts]") {
    std::unique_ptr<tts::sink> previous = tts::set(nullptr);
    tts::reset();

    tts::recording_sink* recorder = dynamic_cast<tts::recording_sink*>(&tts::get());
    REQUIRE(recorder != nullptr);

    // And it is a working destination rather than a hole: an utterance that
    // reaches it can be read back, which is what makes forgetting to install a
    // sink harmless rather than merely quiet.
    tts::output("a prompt raised by somebody else's test");
    REQUIRE(recorder->spoken().size() == 1);
    CHECK(recorder->brailled().size() == 1);

    tts::set(std::move(previous));
}
