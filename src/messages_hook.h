#pragma once

#include <string>

enum game_message_type : int;

namespace cata
{

/**
 * Fire the `on_add_msg` hook for a message that was just added to the log.
 *
 * Params handed to Lua: `text` (the message string) and `type` (a `game_message_type`),
 * so handlers can filter without re-parsing the message.
 *
 * Nested calls are ignored. A handler that adds a message of its own would otherwise
 * loop forever; the nested message still reaches the log, it just does not fire the
 * hook a second time.
 *
 * This lives outside `messages.cpp` because that translation unit is replaced by a
 * stub in the test build (`tests/fake_messages.cpp`), which would make the hook
 * untestable.
 */
void run_on_add_msg_hook( const std::string &text, game_message_type type );

/** Whether the hook is currently running. Exposed for tests. */
bool is_running_on_add_msg_hook();

} // namespace cata
