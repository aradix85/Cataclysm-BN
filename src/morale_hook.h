#pragma once

#include <string>
#include <vector>

#include "string_formatter.h"
#include "translations.h"

namespace cata
{

/**
 * One row of the morale screen, as the screen itself draws it.
 *
 * `value` is the figure drawn at the right of the row, already worded: a
 * percentage for a source, a signed figure for a total. The screen draws a bare
 * dash where such a figure is zero, which is a placeholder for the eye and
 * nothing at all for the ear, so a zero is a zero here.
 *
 * Both halves are built here rather than in script because the screen keeps its
 * text nowhere -- it words every row inside its own redraw and throws it away
 * -- so the closest thing to reading a label off it is asking for the same
 * translated string. That is the only text in the layer taken this way, and it
 * is why the figures are formatted here too: the two belong together.
 */
struct morale_row {
    std::string text;
    std::string value;
};

/**
 * What the screen is told to draw, in the numbers it is given.
 *
 * `pain_penalty` and `fatigue_cap` are drawn only where they apply, and zero is
 * how "does not apply" arrives here. `focus` is always drawn.
 *
 * `negative` is the magnitude the game reports, which the screen draws negated,
 * and `pain_penalty` likewise. Both are taken as the game gives them, so that
 * the caller passes an answer rather than an arrangement of one.
 */
struct morale_summary {
    int positive = 0;
    int negative = 0;
    int total = 0;
    int pain_penalty = 0;
    int fatigue_cap = 0;
    int focus = 0;
};

/** A figure the screen signs, with a zero said as a zero rather than drawn as a dash. */
inline std::string morale_total_text( const int value )
{
    return value == 0 ? std::string( "0" ) : string_format( "%+d", value );
}

/** A share of the whole, in the screen's own format. */
inline std::string morale_percent_text( const double percent )
{
    return string_format( "%d%%", static_cast<int>( percent ) );
}

/**
 * A label the screen draws with a colon after it, which is punctuation for a
 * column of figures and a word the synthesiser reads out loud.
 */
inline std::string morale_label_text( const std::string &label )
{
    if( !label.empty() && label.back() == ':' ) {
        return label.substr( 0, label.size() - 1 );
    }
    return label;
}

/**
 * The middle of the screen as a list of rows: each group's total, and under it
 * every source contributing to that group, in the order the screen sorted them.
 *
 * A template because the points are a type private to `player_morale`, so this
 * can only be built where the screen builds them. Nothing here names that type;
 * a point is anything answering `get_name()` and `get_percent_contribution()`,
 * which is also what lets a test drive this with rows of its own.
 *
 * Empty where nothing affects the character's morale, which is the case the
 * screen answers with a sentence of its own. There is nothing to walk then, and
 * a list of no rows says that better than a row saying there are none.
 */
template<typename Points>
std::vector<morale_row> morale_rows( const Points &positive, const Points &negative,
                                     const morale_summary &at )
{
    std::vector<morale_row> rows;
    if( positive.empty() && negative.empty() ) {
        return rows;
    }

    rows.push_back( morale_row{ _( "Total positive morale" ), morale_total_text( at.positive ) } );
    for( const auto &point : positive ) {
        rows.push_back( morale_row{ point.get_name(),
                                    morale_percent_text( point.get_percent_contribution() ) } );
    }
    rows.push_back( morale_row{ _( "Total negative morale" ), morale_total_text( -at.negative ) } );
    for( const auto &point : negative ) {
        rows.push_back( morale_row{ point.get_name(),
                                    morale_percent_text( point.get_percent_contribution() ) } );
    }
    return rows;
}

/**
 * What the screen prints below the list: the figures that are about the
 * character rather than about one source of morale.
 *
 * The total itself is not here. It is what the screen is for, so it is said
 * when the screen opens rather than waited for at the end of a list.
 */
std::vector<morale_row> morale_summary_rows( const morale_summary &at );

/**
 * Fire the `on_morale` hook for the morale screen that is about to wait for a
 * key.
 *
 * That screen opens on a key in ordinary play, draws its own window and reads
 * its own keys, so nothing else in the layer reaches it: a key was pressed, the
 * screen changed, and the next keypress went into a screen the player did not
 * know was there.
 *
 * Params handed to Lua: `rows`, every row of the list as `{ text, value }`;
 * `total`, the figure the whole screen is about; `summary`, the rows drawn
 * below the list in the same shape; and `action`, what the previous round of
 * this screen's own loop answered -- empty on the firing that is the screen
 * opening.
 *
 * Every row every time, rather than the selected one as the inventory does,
 * because this screen has no selection to hand over. It scrolls a window, and
 * only when the list does not fit, which it usually does -- so its arrow keys
 * answer with nothing at all and no row below the first can be reached. The
 * reading position therefore lives in the layer, which is possible here and was
 * not on the keybindings screen for one reason: no row here can be acted on, so
 * a selection is a place in a reading rather than a thing the screen has to
 * agree about. The screen's own scrolling is left exactly as it was.
 *
 * The list is short by construction -- one row per morale effect the character
 * currently has -- so handing it over on every keypress costs less than drawing
 * the screen does.
 *
 * Fired once per input round, so every keypress the screen ignores arrives here
 * too and the handler must work out what changed.
 */
void fire_on_morale( const std::vector<morale_row> &rows, const morale_summary &at,
                     const std::string &action );

/** The overload the screen calls: the points it has already split and sorted. */
template<typename Points>
void fire_on_morale( const Points &positive, const Points &negative, const morale_summary &at,
                     const std::string &action )
{
    fire_on_morale( morale_rows( positive, negative, at ), at, action );
}

} // namespace cata
