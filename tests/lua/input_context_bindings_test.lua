-- Pins down the Lua-visible names of the command dispatch bindings. The mod's
-- main.lua calls exactly these, so a rename or an arity change in C++ fails
-- here rather than as a silent debugmsg in a game the player cannot read.

local ctxt = InputContext.new("BN_ACCESS_LOOP")
ctxt:register_action("NEXT")
ctxt:register_action("PREV", "Previous")
ctxt:register_directions()

gapi.register_default_mode_action("bn_access_test_action", "Test action")
gapi.register_default_mode_action("bn_access_test_action_unnamed")

test_data.out = {
  known = ctxt:is_action_registered("NEXT"),
  named = ctxt:is_action_registered("PREV"),
  directions = ctxt:is_action_registered("LEFT"),
  unknown = ctxt:is_action_registered("NEVER_REGISTERED"),
}
