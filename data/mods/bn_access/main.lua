-- bn_access: self-voicing accessibility layer for Cataclysm: Bright Nights.
--
-- Loading this mod is silent on purpose. It speaks when the game says something
-- to the player and when the game demands an answer. Nothing else, and nothing
-- is asked of the player first.
--
-- Blocking prompts speak because a prompt accepts its own two or four keys and
-- swallows every other one, so a silent prompt makes every command unreachable.
-- The message log speaks because that is the game telling the player what just
-- happened to them.
--
-- The mod owns one key: the question the game cannot answer by itself. Being
-- told what happened is not the same as being able to ask what is there.

local speech = require("./lib/speech")
local prompts = require("./lib/prompts")
local messages = require("./lib/messages")
local movement = require("./lib/movement")
local surroundings = require("./lib/surroundings")
local text = require("./lib/text")

local mod = game.mod_runtime[game.current_mod]

mod.speech = speech

-- How far the overview looks. Creatures are limited by what the character can
-- actually see; the terrain scan is a square of this radius, so it is also the
-- cost of the command -- keep it small enough that a keypress answers at once.
local RANGE = 12
local LANDMARK_RANGE = 8

gapi.register_default_mode_action("bn_access_surroundings", "Accessibility: what is around me")

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
local open_prompt = nil

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

-- Every attempt to move, including the ones the game refuses. This hook fires
-- before the move resolves, so whether it will be refused is worked out here
-- rather than waited for: an impassable square is one the game will not enter.
--
-- A closed door is impassable and is not a refusal: walking into it opens it.
-- Naming it is right either way, so it needs no special case beyond not calling
-- it blocked.
--
-- Returns nothing. Returning false would veto the move, and this hook only
-- watches.
game.add_hook("on_player_try_move", {
  priority = 100,
  fn = function(params)
    local here = gapi.get_map()
    local to = params.to
    local from = params.from

    local passable = perception.move_cost_at(to) > 0
    local door = here:has_ter_flag_at("DOOR", to)
    local name = square_name(to)

    for _, line in
      ipairs(movement.utterances({
        blocked = not passable and not door,
        name = name,
        changed = name ~= square_name(from),
      }))
    do
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

-- An action in the default mode context can only arrive while no popup holds
-- the keyboard, so the prompt that was open is gone. Forgetting it here is what
-- lets the same question be asked, and answered, twice.
game.add_hook("on_action", {
  priority = 100,
  fn = function(params)
    open_prompt = nil

    if params.action == "bn_access_surroundings" then
      for _, line in ipairs(surroundings.overview(collect())) do
        speech.say(line)
      end
      return false
    end
  end,
})
