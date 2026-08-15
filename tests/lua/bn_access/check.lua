-- Assertions for the layer's Lua, written in Lua.
--
-- Every case the mod's own scripts need used to cost a line of C++ and a link
-- of the test binary, which is the long part of a build. A case written here
-- costs nothing: the harness in tests/bn_access_lua_test.cpp runs every
-- *_test.lua beside this file and reports what they recorded.
--
-- The harness loads this file itself and hands it to each script as the global
-- `check`, so a test script requires nothing to use it.
--
-- It counts as well as compares, because a script that asserts nothing passes
-- in exactly the same way as one that asserts everything. The harness fails a
-- script whose count is zero.

local check = { count = 0, failures = {} }

--- Compare two values, and record what was wrong when they differ.
---
--- The label is what the reader sees when it fails, so write what the case
--- proves rather than what it does: nobody reading a red test has the code in
--- front of them, and the owner of this project reads no code at all.
--- @param actual any
--- @param expected any
--- @param label string
check.equal = function(actual, expected, label)
  check.count = check.count + 1
  if actual == expected then return end
  check.failures[#check.failures + 1] =
    string.format("%s\n    expected: %s\n    actual:   %s", label, tostring(expected), tostring(actual))
end

return check
