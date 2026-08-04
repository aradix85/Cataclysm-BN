-- bn_access: self-voicing accessibility layer for Cataclysm: Bright Nights.
--
-- Loading this mod is silent on purpose. Nothing is announced until the message
-- hook and the command dispatch exist, so the first thing the player ever hears
-- is something that was asked for.

local speech = require( "lib.speech" )

local mod = game.mod_runtime[game.current_mod]

mod.speech = speech
