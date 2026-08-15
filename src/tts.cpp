#include "tts.h"

#include <memory>
#include <string>
#include <utility>

#include "cached_options.h"

#if defined(_WIN32)
#include "catacharset.h"
#include "platform_win.h"
#endif

namespace
{

/** Silent but valid. The default wherever there is no NVDA to talk to. */
class null_sink : public tts::sink
{
    public:
        void speak( const std::string &, tts::priority ) override {}
        void braille( const std::string & ) override {}
        void cancel_speech() override {}
        bool is_available() const override {
            return false;
        }
};

#if defined(_WIN32)

/**
 * Talks to NVDA through nvdaControllerClient.dll.
 *
 * Loaded with LoadLibrary rather than linked, so a missing DLL degrades to
 * silence instead of stopping the game from starting, and no CMake change is
 * needed. The function pointer types are declared here instead of including
 * nvdaController.h: that header lives outside the repository and would need an
 * include path, and all that is wanted from it is the shape of five calls.
 * MIDL's error_status_t is unsigned long and its boolean is unsigned char.
 */
class nvda_sink : public tts::sink
{
        using nvda_error = unsigned long;
        using fn_status = nvda_error( __stdcall * )();
        using fn_text = nvda_error( __stdcall * )( const wchar_t * );
        using fn_ssml = nvda_error( __stdcall * )( const wchar_t *, int, int, unsigned char );

    public:
        nvda_sink() {
            lib_ = LoadLibraryW( L"nvdaControllerClient.dll" );
            if( lib_ == nullptr ) {
                return;
            }
            test_if_running_ = resolve<fn_status>( "nvdaController_testIfRunning" );
            speak_text_ = resolve<fn_text>( "nvdaController_speakText" );
            cancel_speech_ = resolve<fn_status>( "nvdaController_cancelSpeech" );
            braille_message_ = resolve<fn_text>( "nvdaController_brailleMessage" );
            // Only in the NvdaController2 interface, so treat its absence as normal.
            speak_ssml_ = resolve<fn_ssml>( "nvdaController_speakSsml" );
        }

        ~nvda_sink() override {
            if( lib_ != nullptr ) {
                FreeLibrary( lib_ );
            }
        }

        nvda_sink( const nvda_sink & ) = delete;
        nvda_sink &operator=( const nvda_sink & ) = delete;

        void speak( const std::string &text, tts::priority prio ) override {
            if( !loaded() ) {
                return;
            }
            // speakText carries no priority, so anything above normal has to go
            // out as SSML. Without speakSsml the only lever left is cancelling.
            if( prio != tts::priority::normal && speak_ssml_ != nullptr ) {
                const std::string ssml = "<speak>" + tts::escape_ssml( text ) + "</speak>";
                constexpr int symbol_level_unchanged = -1;
                constexpr unsigned char asynchronous = 1;
                const nvda_error err = speak_ssml_( utf8_to_wstr( ssml ).c_str(),
                                                    symbol_level_unchanged,
                                                    static_cast<int>( prio ), asynchronous );
                if( err == 0 ) {
                    return;
                }
                // Resolving the symbol proves nothing: speakSsml is part of
                // controller client 2.0 and NVDA older than 2024.1 answers every
                // call with RPC_S_UNKNOWN_IF (1717). Stop asking and fall through,
                // or every utterance above normal priority would be silent.
                speak_ssml_ = nullptr;
            }
            if( prio == tts::priority::now && cancel_speech_ != nullptr ) {
                cancel_speech_();
            }
            speak_text_( utf8_to_wstr( text ).c_str() );
        }

        void braille( const std::string &text ) override {
            if( loaded() && braille_message_ != nullptr ) {
                braille_message_( utf8_to_wstr( text ).c_str() );
            }
        }

        void cancel_speech() override {
            if( loaded() && cancel_speech_ != nullptr ) {
                cancel_speech_();
            }
        }

        /**
         * Asks NVDA itself, so this costs a round trip. Callers that only need to
         * know whether the DLL resolved should not use it per utterance.
         */
        bool is_available() const override {
            return loaded() && test_if_running_ != nullptr && test_if_running_() == 0;
        }

    private:
        template<typename Fn>
        Fn resolve( const char *name ) const {
            return reinterpret_cast<Fn>( GetProcAddress( lib_, name ) );
        }

        bool loaded() const {
            return lib_ != nullptr && speak_text_ != nullptr;
        }

        HMODULE lib_ = nullptr;
        fn_status test_if_running_ = nullptr;
        fn_text speak_text_ = nullptr;
        fn_status cancel_speech_ = nullptr;
        fn_text braille_message_ = nullptr;
        fn_ssml speak_ssml_ = nullptr;
};

#endif // _WIN32

std::unique_ptr<tts::sink> make_default_sink()
{
    // A test binary talks to a recorder, never to NVDA. The suite loads the
    // layer into the world it builds, and the game's own tests raise real
    // prompts and real messages while they run -- so with the platform sink as
    // the default the machine that runs the suite is spoken to, at length, by
    // nobody's game. That is somebody's screen reader and somebody's braille
    // display, in the middle of whatever they were reading.
    //
    // A recorder rather than silence, so that a test which forgets to install
    // one of its own still has a destination it can read back, and so that the
    // safe default costs a test nothing. Tests that install their own sink are
    // unaffected: this is only what `get()` answers when nothing was installed.
    if( test_mode ) {
        return std::make_unique<tts::recording_sink>();
    }
#if defined(_WIN32)
    return std::make_unique<nvda_sink>();
#else
    return std::make_unique<null_sink>();
#endif
}

std::unique_ptr<tts::sink> &current_sink()
{
    static std::unique_ptr<tts::sink> s = make_default_sink();
    return s;
}

} // namespace

namespace tts
{

sink &get()
{
    return *current_sink();
}

priority priority_from_int( int value )
{
    switch( value ) {
        case static_cast<int>( priority::next ):
            return priority::next;
        case static_cast<int>( priority::now ):
            return priority::now;
        default:
            return priority::normal;
    }
}

void output( const std::string &text, priority prio )
{
    output( text, text, prio );
}

void output( const std::string &spoken, const std::string &brailled, priority prio )
{
    sink &s = get();
    s.speak( spoken, prio );
    s.braille( brailled );
}

std::unique_ptr<sink> set( std::unique_ptr<sink> s )
{
    if( s == nullptr ) {
        s = make_default_sink();
    }
    std::unique_ptr<sink> previous = std::move( current_sink() );
    current_sink() = std::move( s );
    return previous;
}

void reset()
{
    current_sink() = make_default_sink();
}

void recording_sink::speak( const std::string &text, priority prio )
{
    spoken_.push_back( utterance{ text, prio } );
}

void recording_sink::braille( const std::string &text )
{
    brailled_.push_back( text );
}

void recording_sink::cancel_speech()
{
    cancels_ += 1;
}

bool recording_sink::is_available() const
{
    return true;
}

void recording_sink::clear()
{
    spoken_.clear();
    brailled_.clear();
    cancels_ = 0;
}

std::string escape_ssml( const std::string &text )
{
    std::string out;
    out.reserve( text.size() );
    for( const char c : text ) {
        switch( c ) {
            case '&':
                out += "&amp;";
                break;
            case '<':
                out += "&lt;";
                break;
            case '>':
                out += "&gt;";
                break;
            case '"':
                out += "&quot;";
                break;
            case '\'':
                out += "&apos;";
                break;
            default:
                out += c;
                break;
        }
    }
    return out;
}

} // namespace tts
