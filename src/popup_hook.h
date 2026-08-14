#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace cata
{

/**
 * Fire the `on_query_popup` hook for a blocking prompt that is about to wait
 * for a key.
 *
 * Params handed to Lua: `text` (the message as displayed), `category` (the
 * popup's input context category, such as "YESNO"), `options` (a sequence of
 * `{ id, name }` tables, `name` being the translated action name the popup
 * itself shows) and `cursor` (the 1-based index of the selected option, absent
 * when the popup offers none).
 *
 * Fired once per input round rather than once per popup, so moving between
 * options fires it again with a different `cursor`. Which part of that is worth
 * speaking is the handler's decision, not this function's.
 *
 * Nested calls are ignored: a handler that trips a debugmsg raises a popup of
 * its own, which would otherwise recurse until the stack runs out.
 */
void run_on_query_popup_hook( const std::string &category, const std::string &text,
                              const std::vector<std::string> &option_actions, std::size_t cursor );

/**
 * Adapter for the popup's own option list, whose element type is private to
 * `query_popup`. Only the public `action` member is read, so the type never has
 * to be named. It lives here rather than in `popup.cpp` so that the call site
 * there stays a single line and every decision about what the hook carries
 * stays in a file a test can reach.
 */
template <typename Options>
void fire_on_query_popup( const std::string &category, const std::string &text,
                          const Options &options, std::size_t cursor )
{
    std::vector<std::string> option_actions;
    option_actions.reserve( options.size() );
    for( const auto &opt : options ) {
        option_actions.push_back( opt.action );
    }
    run_on_query_popup_hook( category, text, option_actions, cursor );
}

/** Whether the hook is currently running. Exposed for tests. */
bool is_running_on_query_popup_hook();

} // namespace cata
