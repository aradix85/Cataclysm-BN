#pragma once

#include <string>

namespace cata
{

/**
 * Fire the `on_debugmsg` hook for the game's own error report, just before that
 * report takes the keyboard.
 *
 * Params handed to Lua: `text`, the error itself, without the function, file,
 * line and version the report draws around it.
 *
 * The screen this belongs to reads raw input and accepts only space, `i` and
 * `c`, swallowing every other key until one of those arrives. So a handler
 * cannot register a key of its own here and gets one firing in which to say
 * everything it has to say, the way out included.
 *
 * Fired once per report shown, which includes the reports buffered during
 * startup and the ones queued by worker threads, both of which are replayed on
 * the main thread. A report the player has chosen to ignore is not shown and
 * does not fire.
 *
 * Nested calls are ignored. A handler that fails is itself reported by
 * `debugmsg`, which raises this screen again; without the guard that second
 * report would call the same failing handler.
 */
void run_on_debugmsg_hook( const std::string &text );

/** Whether the hook is currently running. Exposed for tests. */
bool is_running_on_debugmsg_hook();

} // namespace cata
