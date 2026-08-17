-- bn_access: self-voicing accessibility layer for Cataclysm: Bright Nights.
--
-- Loaded by the fork rather than by the mod system, so it is alive from the
-- first screen to the last and cannot be switched off in a list. See
-- src/access_layer.h for why, and for the two lives this file has.
--
-- Coming up is silent until the world is ready, and then it says so once.
-- Before that word, silence means the game is still loading; after it, silence
-- means something is wrong. Without that line the two are the same sound, and a
-- key pressed during a load that takes seconds is answered by nothing at all --
-- which is what pressing harder is a response to.
--
-- Nothing else announces itself. It speaks when the game says something to the
-- player and when the game demands an answer.
--
-- Blocking prompts speak because a prompt accepts its own two or four keys and
-- swallows every other one, so a silent prompt makes every command unreachable.
-- The message log speaks because that is the game telling the player what just
-- happened to them. Menus speak because a menu holds the keyboard the same way
-- a prompt does, and because the game's own action menu lists every verb in the
-- game, including this layer's own commands. The screen the game opens on
-- speaks because it is the only way into a world and the way back out of one.
--
-- The layer owns one key: the question the game cannot answer by itself. Being
-- told what happened is not the same as being able to ask what is there.

local speech = require("./lib/speech")
local errors = require("./lib/errors")
local prompts = require("./lib/prompts")
local menus = require("./lib/menus")
local inventory = require("./lib/inventory")
local advanced_inventory = require("./lib/advanced_inventory")
local describe = require("./lib/describe")
local keybindings = require("./lib/keybindings")
local opening = require("./lib/opening")
local look = require("./lib/look")
local nearby = require("./lib/nearby")
local overmap = require("./lib/overmap")
local play = require("./lib/play")
local messages = require("./lib/messages")
local movement = require("./lib/movement")
local surroundings = require("./lib/surroundings")
local text = require("./lib/text")
local zone = require("./lib/zone")

-- No mod table here, and no mod id: the fork loads this file itself, into every
-- Lua state that exists, so that the layer is alive on the screens that come
-- before a world and on the ones that come after leaving it.
--
-- The two lives differ in one way only. Loaded before a world, there is nothing
-- loading and nothing to say about a world, and the one thing worth knowing is
-- that the layer is there at all -- said before the opening screen has drawn
-- itself, so that a screen which then names itself is confirmation rather than
-- the first sign of anything.
-- Set by src/access_layer.cpp before this file runs, so the analyzer cannot see
-- where it comes from.
---@diagnostic disable-next-line: undefined-global
local at_boot = access_is_boot == true

if at_boot then
  speech.say("Accessibility on.")

  -- The wait for a world begins the moment the player picks one, and this file
  -- is not read again until the far end of it: the state a world gets is built
  -- by the very load that has to be announced. So the word that opens the wait
  -- comes from here, where something is already alive to say it, and the word
  -- that closes it comes from inside the world. Between the two, silence is the
  -- wait and not a fault.
  game.add_hook("on_world_loading", {
    priority = 100,
    fn = function()
      speech.say("Loading.")
    end,
  })
end

-- The game's own error report, first of everything, because it is how a fault
-- anywhere else -- in the game, in this file, in a hook below -- reaches the
-- player. Registering it here means the rest of this file is already covered
-- while it is still being read.
--
-- The report before this one, so a fault that repeats every turn does not read
-- itself out every turn.
local last_error = nil

game.add_hook("on_debugmsg", {
  priority = 100,
  fn = function(params)
    for _, line in ipairs(errors.utterances(params.text, last_error)) do
      speech.say(line)
    end
    last_error = params.text
  end,
})

-- How far the overview looks. Creatures are limited by what the character can
-- actually see; the terrain scan is a square of this radius, so it is also the
-- cost of the command -- keep it small enough that a keypress answers at once.
local RANGE = 12
local LANDMARK_RANGE = 8
-- How far the room is measured in each direction. A room is ten squares across at
-- most, so beyond this the answer stops being about the room she is standing in.
local WALL_RANGE = 12
-- How far stairs are looked for. Further than a door, because a staircase is worth
-- crossing a room for and is the only thing nearby that leads out of this place
-- altogether.
local STAIR_RANGE = 24

gapi.register_default_mode_action("bn_access_surroundings", "Accessibility: what is around me")
gapi.register_default_mode_action("bn_access_zone", "Accessibility: where am I")

-- The world is ready and the game is about to read a key. A new game reaches
-- this through on_game_started and a loaded save through on_game_load, and a
-- save fires the load hook twice, so the word is said once and only once.
--
-- The Lua state is rebuilt per world, so this resets by itself when another
-- world is loaded in the same session.
local said_ready = false

local function announce_ready()
  if said_ready then return end
  said_ready = true
  speech.say("Accessibility ready.")
end

game.add_hook("on_game_started", { priority = 100, fn = announce_ready })
game.add_hook("on_game_load", { priority = 100, fn = announce_ready })

-- The message log speaks. Registered at a high priority so the layer sees the
-- message before another mod can alter or veto it.
--
-- Everything goes out in the order it happened, at one priority. A run of short
-- sentences does not need a queue, and a message that jumps ahead of another can
-- discard it at NVDA rather than merely overtake it.
game.add_hook("on_add_msg", {
  priority = 100,
  fn = function(params)
    local body = text.clean(params.text)
    if not messages.should_speak(params.type, body) then return end
    speech.say(body)
  end,
})

-- The prompt that is currently on screen, as prompts.state() normalised it.
-- It is what makes "has anything changed" answerable, since the hook fires
-- again after every keypress the prompt did not accept.
--
-- The menu and the opening screen are kept the same way, and the three are
-- exclusive: whichever of them fires holds the keyboard, so the other two are
-- gone and are forgotten here. Forgetting is what lets the same screen be met a
-- second time and still speak -- a screen is recognised by what it says it is,
-- so returning to one is otherwise indistinguishable from never having left it.
local open_prompt = nil
local open_menu = nil
local open_screen = nil

-- A blocking prompt speaks itself. By the time this fires the game has already
-- taken the keyboard, so an announcement is not an interruption, it is the only
-- way out.
game.add_hook("on_query_popup", {
  priority = 100,
  fn = function(params)
    local state = prompts.state(params)
    for _, line in ipairs(prompts.utterances(state, open_prompt)) do
      speech.say(line)
    end
    open_prompt = state
    open_menu = nil
    open_screen = nil
  end,
})

-- The menu that is on screen, as menus.state() normalised it, and the same
-- answer to the same question: the hook fires again after every keypress, so
-- what changed is only knowable by keeping what came before.
--
-- Every uilist in the game arrives here: the menu ESC opens, the action menu on
-- RETURN, the item action and examine menus, the vehicle controls. None of them
-- is followable by any other means -- upstream's cursor support was never
-- ported into uilist -- so this hook is the only way any of them is playable.
local function speak_menu(params)
  local state = menus.state(params)
  for _, line in ipairs(menus.utterances(state, open_menu)) do
    speech.say(line)
  end
  open_menu = state
  open_prompt = nil
  open_screen = nil
end

game.add_hook("on_uilist", { priority = 100, fn = speak_menu })

-- The inventory screens, which are not uilists and need a firing point of their
-- own -- see src/inventory_hook.h. Every one of them waits for a key in the same
-- function, so the plain inventory, wield, wear, eat, drop, pick up and use all
-- arrive here.
--
-- This is the system that had nothing at all: no hook, no message site, no
-- cursor support upstream, so a player without sight opened it and heard
-- silence whatever they pressed.
--
-- Its state goes into the same variable as the menu's, because whichever of the
-- two is on screen holds the keyboard and they cannot both be open. It matters
-- most on the way back: the action menu opens the inventory, and closing the
-- inventory leaves that menu underneath, which then announces itself again
-- because what is remembered is no longer what is on screen.
game.add_hook("on_inventory", {
  priority = 100,
  fn = function(params)
    local state = inventory.state(params)
    for _, line in ipairs(inventory.utterances(state, open_menu)) do
      speech.say(line)
    end
    open_menu = state
    open_prompt = nil
    open_screen = nil
  end,
})

-- The advanced inventory, the screen items are moved on: two panes side by
-- side, one holding the cursor and one receiving. It is not built on
-- inventory_selector and it is not a uilist, so it needs a firing point of its
-- own -- see src/advanced_inventory_hook.h.
--
-- Everything that screen is is drawn rather than written: which pane is active,
-- which square each is aimed at, which square a row came from. So the pair of
-- places is said whenever it changes, and the wording lives in
-- lib/advanced_inventory.lua, composing the menu's reading model.
--
-- Its state goes into the same variable as the menu's, for the reason the
-- inventory's does: whichever screen is on top holds the keyboard, and arriving
-- back at the one underneath is arriving.
game.add_hook("on_advanced_inventory", {
  priority = 100,
  fn = function(params)
    local state = advanced_inventory.state(params)
    for _, line in ipairs(advanced_inventory.utterances(state, open_menu)) do
      speech.say(line)
    end
    open_menu = state
    open_prompt = nil
    open_screen = nil
  end,
})

-- The game's own key list, which every screen but one opens on the question
-- mark, and which is also where a key is rebound. It is not a uilist and needs
-- a firing point of its own, but it is a titled list with a position in it, so
-- it is read by the same model rather than a second one: the C++ side hands it
-- over in the same shape and this is the same handler.
--
-- Deliberately sharing the menu's state and not a variable of its own. Whichever
-- of the two is on screen holds the keyboard, so they cannot both be open, and
-- closing this screen leaves the menu underneath to announce itself again --
-- which is right, because arriving back somewhere is arriving.
--
-- What it took to make it readable: that screen had no selection at all. It
-- scrolled a window and acted on a hotkey letter, and refused to scroll a list
-- that already fits -- so on most screens every arrow key was answered by
-- silence and no row below the first could be reached. The fork gives it a
-- selection that the window follows; see src/keybindings_hook.h.
--
-- The wording is lib/keybindings.lua, which composes the menu's reading model
-- rather than repeating it. Its state goes into the same variable, because it
-- is a menu state with two extra fields the menu module ignores.
game.add_hook("on_keybindings", {
  priority = 100,
  fn = function(params)
    local state = keybindings.state(params)
    for _, line in ipairs(keybindings.utterances(state, open_menu)) do
      speech.say(line)
    end
    open_menu = state
    open_prompt = nil
    open_screen = nil
  end,
})

-- The screen the game opens on, which is not a uilist and needs a firing point
-- of its own. It is the first thing a player meets, the only way into a world,
-- and the screen they are returned to when they leave one.
--
-- Two lists at once, and only what moved is spoken; the wording lives in
-- lib/opening.lua.
game.add_hook("on_main_menu", {
  priority = 100,
  fn = function(params)
    local state = opening.state(params)
    for _, line in ipairs(opening.utterances(state, open_screen)) do
      speech.say(line)
    end
    open_screen = state
    open_prompt = nil
    open_menu = nil
  end,
})

-- The overmap, which is where the world stops being squares and becomes named
-- places: the game describes a tile as "house in central Springfield" and its own
-- travel key walks the character there. It is not a uilist and needs a firing
-- point of its own -- see src/overmap_hook.h.
--
-- Its state goes into the same variable as the menu's, because whichever screen
-- is on top holds the keyboard and they cannot both be open. The wording lives
-- in lib/overmap.lua, which ignores a remembered state belonging to another
-- screen: closing a menu drawn over the map is arriving back at the map.
game.add_hook("on_overmap", {
  priority = 100,
  fn = function(params)
    local state = overmap.state(params)
    for _, line in ipairs(overmap.utterances(state, open_menu)) do
      speech.say(line)
    end
    open_menu = state
    open_prompt = nil
    open_screen = nil
  end,
})

-- The world is about to read a key, which is also the moment a screen that was on
-- top has stopped answering them. Every screen in the game closes silently, so
-- this is where that silence is broken -- once, for all of them at once, rather
-- than at the bottom of each screen's own loop. See src/play_hook.h.
--
-- What was on top is forgotten here, so returning to the same screen later reads
-- as arriving there again.
game.add_hook("on_play_input", {
  priority = 100,
  fn = function()
    for _, line in ipairs(play.utterances({ screen = open_menu ~= nil or open_screen ~= nil })) do
      speech.say(line)
    end
    open_prompt = nil
    open_menu = nil
    open_screen = nil
  end,
})

-- The game's own list of every item in view, on its own key. Not a uilist and not
-- built on the inventory, so it needs a firing point of its own -- see
-- src/nearby_hook.h.
--
-- It answers the question the look-around cursor cannot: where is there anything
-- at all, without walking a cursor over every square. The wording composes the
-- menu's reading model and adds the one thing this screen exists for, which is
-- where each item is.
--
-- Its state goes into the same variable as the menu's, because whichever screen is
-- on top holds the keyboard and they cannot both be open.
game.add_hook("on_nearby_items", {
  priority = 100,
  fn = function(params)
    local state = nearby.state(params)
    for _, line in ipairs(nearby.utterances(state, open_menu)) do
      speech.say(line)
    end
    open_menu = state
    open_prompt = nil
    open_screen = nil
  end,
})

-- The game's own detail screen, which the look-around cursor opens on its describe
-- key. Not a uilist, so it needs a firing point of its own -- see
-- src/description_hook.h.
--
-- It is the level of detail P1 keeps out of the per-square line: the full
-- description of what is there, said in pieces so speech can be cut off, and only
-- when it changes.
--
-- Its state goes into the same variable as the menu's, because whichever screen is
-- on top holds the keyboard and they cannot both be open.
game.add_hook("on_description", {
  priority = 100,
  fn = function(params)
    local state = describe.state(params)
    for _, line in ipairs(describe.utterances(state, open_menu)) do
      speech.say(line)
    end
    open_menu = state
    open_prompt = nil
    open_screen = nil
  end,
})

-- The name of what is on a square: furniture if there is any, since a stove on
-- a floor is a stove, and the terrain otherwise.
local function square_name(pos)
  local here = gapi.get_map()
  local furn = here:get_furn_at(pos)
  if not furn:str_id():is_null() then return furn:obj():name() end
  return here:get_ter_at(pos):obj():name()
end

-- The look-around cursor, which is the game's own way of asking about one square
-- rather than about everything at once. Not a uilist, so it needs a firing point
-- of its own -- see src/look_hook.h.
--
-- Everything the screen prints about a square is gathered here, which is what
-- keeps lib/look.lua pure. Only sight comes from the firing point: how much of a
-- square the character can make out is not something script can work out.
--
-- Its state goes into the same variable as the menu's, because whichever screen is
-- on top holds the keyboard and they cannot both be open.
game.add_hook("on_look_around", {
  priority = 100,
  fn = function(params)
    local you = gapi.get_avatar()
    local here = gapi.get_map()
    local at = you:get_pos_ms()
    local cursor = params.cursor

    -- Hallucinations are excluded, as the game's own panel excludes them. The
    -- character herself is excluded too, and that is not the same thing: the cursor
    -- opens on her own square, the game counts her as the creature standing there,
    -- and the square would answer with her own name read out as a stranger. Told
    -- apart by the square rather than by the name, since two characters can share
    -- one name and only one of them can stand on her feet.
    local standing_here = cursor.x == at.x and cursor.y == at.y and cursor.z == at.z
    local critter = not standing_here and gapi.get_creature_at(cursor, false) or nil
    local sensed = {}
    -- A creature the eyes cannot reach but a sense can: infrared through
    -- darkness, the special senses through walls. This is F3 itself -- the
    -- mechanic works and was invisible to the player -- so it is said in the
    -- game's own words rather than left out.
    if critter and params.sight ~= "clear" then
      if perception.sees_with_infrared(you, critter) then
        sensed = perception.describe_infrared(critter)
      elseif perception.sees_with_specials(you, critter) then
        sensed = perception.describe_specials(critter)
      end
    end

    local state = look.state({
      sight = params.sight,
      name = square_name(cursor),
      area = perception.area_name_at(cursor),
      creature = critter and critter:get_name() or "",
      sensed = sensed,
      sound = perception.sound_at(cursor),
      items = #here:get_items_at(cursor),
      dx = cursor.x - at.x,
      dy = cursor.y - at.y,
      dz = cursor.z - at.z,
      peeking = params.peeking,
    })

    for _, line in ipairs(look.utterances(state, open_menu)) do
      speech.say(line)
    end
    open_menu = state
    open_prompt = nil
    open_screen = nil
  end,
})

-- A step the game is willing to take. The square is one it will enter, so this
-- is where the ground underfoot is named, and only when it changes.
--
-- Returns nothing. Returning false would veto the move, and this hook only
-- watches.
game.add_hook("on_player_try_move", {
  priority = 100,
  fn = function(params)
    local name = square_name(params.to)
    -- Leaving one region of the overmap for another, which the game names and
    -- says nowhere in play: without this the only way to learn that she has
    -- walked into the cabin she was heading for is to open the map. Compared by
    -- name and not by coordinate, because the name is what she would hear and two
    -- stretches of forest meeting is not an arrival.
    local area = perception.area_name_at(params.to)
    local step = {
      name = name,
      changed = name ~= square_name(params.from),
      area = area ~= perception.area_name_at(params.from) and area or "",
    }

    for _, line in ipairs(movement.utterances(step)) do
      speech.say(line)
    end
  end,
})

-- A step the game refuses in silence, which is the wall. An impassable square is
-- dropped long before on_player_try_move fires, so a refusal needs a firing point
-- of its own, and the obstacle is named by the game rather than looked up here:
-- that name covers a vehicle in the way, which terrain and furniture do not.
--
-- A refusal on a square that can be entered came from something the game did
-- explain -- a vehicle that cannot be boarded while riding, a creature that
-- cannot be dragged -- and that explanation arrives as a message and is spoken
-- there. So only an impassable square answers here.
game.add_hook("on_player_move_refused", {
  priority = 100,
  fn = function(params)
    if perception.move_cost_at(params.to) > 0 then return end

    for _, line in ipairs(movement.utterances({ blocked = true, name = params.obstacle })) do
      speech.say(line)
    end
  end,
})

-- How far the room goes in one direction: how many squares can be walked that way
-- before something stops her, and whether anything did within the distance looked
-- at. A wall, a counter, a locked door and a vehicle all stop her alike, which is
-- what makes move cost the right question -- it is what the game itself asks before
-- letting a step happen.
--
-- Bounded by WALL_RANGE, because the answer is about the room she is in rather than
-- about the horizon, and an unbounded scan across open ground would cost a keypress
-- more than it is worth.
local function arm_of(at, dx, dy)
  for steps = 1, WALL_RANGE do
    if
      perception.move_cost_at(coords.tripoint_bub_ms(at.x + dx * steps, at.y + dy * steps, at.z)) == 0
    then
      return { steps = steps - 1, blocked = true }
    end
  end
  return { steps = WALL_RANGE, blocked = false }
end

-- Gathering what is around the player. The only impure part of the layer:
-- everything it produces is plain numbers and strings, which is what lets the
-- wording be asserted without a game.
local function collect()
  local you = gapi.get_avatar()
  local here = gapi.get_map()
  local at = you:get_pos_ms()

  local function offset(pos) return pos.x - at.x, pos.y - at.y end

  local enemies, others, landmarks = {}, {}, {}
  local ways_up, ways_down = {}, {}

  local hostile_at = {}
  for _, critter in ipairs(you:get_hostile_creatures(RANGE)) do
    local pos = critter:get_pos_ms()
    hostile_at[pos.x .. ":" .. pos.y] = true
    local dx, dy = offset(pos)
    enemies[#enemies + 1] = { name = critter:get_name(), dx = dx, dy = dy }
  end

  for _, critter in ipairs(you:get_visible_creatures(RANGE)) do
    local pos = critter:get_pos_ms()
    if not hostile_at[pos.x .. ":" .. pos.y] then
      local dx, dy = offset(pos)
      others[#others + 1] = { name = critter:get_name(), dx = dx, dy = dy }
    end
  end

  -- A door is the thing you need and cannot see: it is how you leave a room.
  for _, pos in ipairs(here:points_in_radius(at, LANDMARK_RANGE)) do
    if here:has_ter_flag_at("DOOR", pos) then
      local dx, dy = offset(pos)
      if not (dx == 0 and dy == 0) then
        landmarks[#landmarks + 1] = { name = here:get_ter_at(pos):obj():name(), dx = dx, dy = dy }
      end
    end
  end

  -- Stairs are the other way out, and the one that leads somewhere else entirely:
  -- a floor up or down is usually another place with another name. Looked for
  -- further away than a door, because a staircase is worth walking to and a door
  -- twenty squares off belongs to a room she is not in.
  for _, pos in ipairs(here:points_in_radius(at, STAIR_RANGE)) do
    local dx, dy = offset(pos)
    if here:has_ter_flag_at("GOES_UP", pos) then
      ways_up[#ways_up + 1] = { name = here:get_ter_at(pos):obj():name(), dx = dx, dy = dy }
    elseif here:has_ter_flag_at("GOES_DOWN", pos) then
      ways_down[#ways_down + 1] = { name = here:get_ter_at(pos):obj():name(), dx = dx, dy = dy }
    end
  end

  return {
    area = perception.area_name_at(at),
    -- Which way the room lets her walk, and how far. Four directions and not eight:
    -- a frame has to be held in the head while she walks it, and four is what fits.
    reach = {
      north = arm_of(at, 0, -1),
      east = arm_of(at, 1, 0),
      south = arm_of(at, 0, 1),
      west = arm_of(at, -1, 0),
    },
    enemies = enemies,
    others = others,
    landmarks = landmarks,
    ways_up = ways_up,
    ways_down = ways_down,
  }
end

-- An action in the default mode context can only arrive while no popup, no menu
-- and no opening screen holds the keyboard, so all three are gone and are
-- forgotten here, for the reason given where they are declared.
game.add_hook("on_action", {
  priority = 100,
  fn = function(params)
    open_prompt = nil
    open_menu = nil
    open_screen = nil

    if params.action == "bn_access_surroundings" then
      for _, line in ipairs(surroundings.overview(collect())) do
        speech.say(line)
      end
      return false
    end

    -- Where she has ended up, rather than what is within reach of her. Asked once
    -- on arriving somewhere and again when deciding whether this place is done,
    -- which is why it is not folded into the answer above: that one is pressed
    -- every few steps and would carry the size of a station she already knows.
    if params.action == "bn_access_zone" then
      for _, line in ipairs(zone.utterances(zone.state(perception.zone_around_player()))) do
        speech.say(line)
      end
      return false
    end
  end,
})
