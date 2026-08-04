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
 * The sink is replaceable at runtime rather than behind a TESTS compile flag, so
 * the path exercised by tests is the same path that ships.
 */
namespace tts
{

/**
 * Where an utterance goes relative to speech already queued inside NVDA.
 *
 * Values match SPEECH_PRIORITY in nvdaController.h. They are restated here so
 * that nothing outside the Windows sink has to include that header.
 */
enum class priority : int {
    /** Queue behind everything already queued. */
    normal = 0,
    /** Speak after the current utterance, ahead of the rest of the queue. */
    next = 1,
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
 * Say something on both channels. This is the entry point to use.
 *
 * Braille is a first-class channel here, not an afterthought, so the ordinary
 * call reaches both and neither can be forgotten by omission.
 *
 * The one-string form sends the same text to speech and braille, which is how
 * a screen reader normally behaves. **Whether any utterance should differ
 * between the two is an open question** — the owner will settle it by reading,
 * and until then nothing in this layer may assume it either way. The two-string
 * overload exists so that answer can be acted on without reshaping call sites,
 * not because a difference is expected.
 */
void output( const std::string &text, priority prio = priority::normal );
void output( const std::string &spoken, const std::string &brailled, priority prio );

/** Install a sink, returning the one it replaced so a caller can restore it. */
std::unique_ptr<sink> set( std::unique_ptr<sink> s );

/** Restore the platform default sink. */
void reset();

/**
 * Escape text for inclusion in an SSML document.
 *
 * The only string transformation the bridge performs itself, and the reason
 * priority works at all: nvdaController_speakText takes no priority, so anything
 * other than normal priority has to go out as SSML. Pure, and unit tested.
 */
std::string escape_ssml( const std::string &text );

} // namespace tts
