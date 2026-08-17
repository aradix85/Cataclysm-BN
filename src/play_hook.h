#pragma once

namespace cata
{

/**
 * Fire the `on_play_input` hook for the main loop that is about to read a key in
 * ordinary play.
 *
 * No params: what is worth saying at this moment depends on what the player has
 * just come back from, and script is what knows that.
 *
 * Every screen in the game closes in silence. The screen itself stops answering
 * keys, the world starts answering them again, and nothing marks the change --
 * which is fine while it can be seen and is not fine at all otherwise: a key
 * pressed after a menu that may or may not still be open goes somewhere the
 * player cannot predict. This is the one place where the answer is the same for
 * every screen there is, so it is answered once here rather than at the bottom of
 * each of them.
 *
 * Fired once per input round rather than once per turn: `game::get_player_input`
 * is called again for every action the player takes, and its own waiting loops
 * for weather and text animation run inside it, below this. So a handler is not
 * asked anything while an animation ticks.
 *
 * Not fired by the activity poll, which reads keys in a loop of its own while an
 * activity or a long walk runs. Nothing is closed there and the world already has
 * the keyboard.
 */
void fire_on_play_input();

} // namespace cata
