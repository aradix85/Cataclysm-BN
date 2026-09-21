#pragma once

#include <memory>
#include <string>
#include <vector>

/**
 * Speech output for the bn_access accessibility layer.
 *
 * Everything the layer says leaves the game through tts::get(). The default sink
 * talks to NVDA through the NVDA Controller Client DLL, resolved at runtime; on
 * other platforms, and when the DLL or NVDA is absent, the default sink stays
 * valid and silent rather than failing.
 *
 * In a test binary the default is a recording_sink instead, so that the suite
 * speaks to nobody: it loads the layer into the world it builds, and the game's
 * own tests raise prompts and messages that the layer would otherwise say out
 * loud on whatever machine is running them.
 *
 * The sink is replaceable at runtime rather than behind a TESTS compile flag, so
 * the path exercised by tests is the same path that ships.
 */
namespace tts
{

/**
 * Where an utterance goes relative to speech already queued inside NVDA.
 *
 * Two values, and that is on purpose. NVDA's own `next` priority is not
 * offered: it can discard speech that is already waiting rather than
 * overtaking it, which is what P8 rules out. It was the only reason this bridge
 * ever built SSML, so `now` is a cancel followed by a speak and `normal` is a
 * plain speak.
 *
 * The values match SPEECH_PRIORITY in nvdaController.h, restated here so
 * that nothing outside the Windows sink has to include that header.
 */
enum class priority : int {
    /** Queue behind everything already queued. */
    normal = 0,
    /** Interrupt the current utterance and speak at once. */
    now = 2,
};

/** One utterance, exactly as it was handed to the sink. */
struct utterance {
    std::string text;
    priority prio = priority::normal;
};

/** Destination for everything the accessibility layer says. */
class sink
{
    public:
        virtual ~sink() = default;

        /** Speak UTF-8 text. */
        virtual void speak( const std::string &text, priority prio ) = 0;
        /** Show UTF-8 text on a braille display as a flash message. */
        virtual void braille( const std::string &text ) = 0;
        /** Drop queued speech and stop speaking now. */
        virtual void cancel_speech() = 0;
        /** Whether this sink can currently deliver anything at all. */
        virtual bool is_available() const = 0;
};

/**
 * A sink that keeps what it was given instead of speaking it.
 *
 * Used by tests, and usable by any code that needs to assert on what would have
 * been said. Recording, not suppressing: it is a destination like any other.
 */
class recording_sink : public sink
{
    public:
        void speak( const std::string &text, priority prio ) override;
        void braille( const std::string &text ) override;
        void cancel_speech() override;
        bool is_available() const override;

        const std::vector<utterance> &spoken() const {
            return spoken_;
        }
        const std::vector<std::string> &brailled() const {
            return brailled_;
        }
        /** Number of cancel_speech() calls since the last clear(). */
        int cancels() const {
            return cancels_;
        }
        void clear();

    private:
        std::vector<utterance> spoken_;
        std::vector<std::string> brailled_;
        int cancels_ = 0;
};

/** The sink in use. Never null. */
sink &get();

/**
 * Turn a number handed in from Lua into a priority.
 *
 * Anything outside the three known values becomes `normal`. A script that passes
 * nonsense should get an ordinary utterance, not undefined behaviour and not
 * silence — the player would have no way to tell which had happened.
 */
priority priority_from_int( int value );

/**
 * Say something on both channels. This is the entry point to use.
 *
 * Braille is a first-class channel here, not an afterthought, so the ordinary
 * call reaches both and neither can be forgotten by omission.
 *
 * **Speech and braille carry the same text.** The owner reads braille and
 * listens at once, so two forms that can drift apart are two forms that can no
 * longer be checked against each other.
 *
 * The two-string overload is the exception and **needs a reason at the call
 * site**. It is for a written form that is a different thing, not a shorter
 * wording of the same thing: "4 NE" beside "four northeast" is not an exception,
 * it quietly demotes braille to a summary of what the ears already had.
 */
void output( const std::string &text, priority prio = priority::normal );
void output( const std::string &spoken, const std::string &brailled, priority prio );

/** Install a sink, returning the one it replaced so a caller can restore it. */
std::unique_ptr<sink> set( std::unique_ptr<sink> s );

/** Restore the platform default sink. */
void reset();

} // namespace tts
