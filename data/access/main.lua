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
local keybindings = require("./lib/keybindings")
local opening = require("./lib/opening")
local messages = require("./lib/messages")
local movement = require("./lib/movement")
local surroundings = require("./lib/surroundings")
local text = require("./lib/text")

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
else
  -- This file runs while the world is being built, which takes about twenty
  -- seconds of data loading and mapgen and says nothing at all. One word here
  -- and one when the world is ready turn that silence into a bracket: this is
  -- the start of the wait, "ready" is the end of it, and neither is an error.
  speech.say("Loading.")
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

gapi.register_default_mode_action("bn_access_surroundings", "Accessibility: what is around me")

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

-- The name of what is on a square: furniture if there is any, since a stove on
-- a floor is a stove, and the terrain otherwise.
local function square_name(pos)
  local here = gapi.get_map()
  local furn = here:get_furn_at(pos)
  if not furn:str_id():is_null() then return furn:obj():name() end
  return here:get_ter_at(pos):obj():name()
end

-- A step the game is willing to take. The square is one it will enter, so this
-- is where the ground underfoot is named, and only when it changes.
--
-- Returns nothing. Returning false would veto the move, and this hook only
-- watches.
game.add_hook("on_player_try_move", {
  priority = 100,
  fn = function(params)
    local name = square_name(params.to)

    for _, line in ipairs(movement.utterances({ name = name, changed = name ~= square_name(params.from) })) do
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

-- Gathering what is around the player. The only impure part of the layer:
-- everything it produces is plain numbers and strings, which is what lets the
-- wording be asserted without a game.
local function collect()
  local you = gapi.get_avatar()
  local here = gapi.get_map()
  local at = you:get_pos_ms()

  local function offset(pos) return pos.x - at.x, pos.y - at.y end

  local enemies, others, landmarks = {}, {}, {}

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

  return { enemies = enemies, others = others, landmarks = landmarks }
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
  end,
})
