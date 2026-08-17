-- Text that is drawn versus text that is said.
--
-- Shared by everything in the layer, because the game hands the same kind of
-- string to a prompt, a message and later a menu: translated prose with colour
-- markup in it, wrapped for a terminal.

local text = {}

--- Strip markup and collapse the whitespace that line wrapping leaves behind.
--- @param s string|nil
--- @return string
text.clean = function(s)
  if not s or s == "" then return "" end
  local out = s:gsub("</?color[^>]*>", "")
  out = out:gsub("%s+", " ")
  out = out:gsub("^ ", ""):gsub(" $", "")
  return out
end

--- Whether anything worth hearing is left.
---
--- Issue #55436: a screen reader handed a line of ASCII decoration reads out
--- the glyphs one by one -- "slash slash slash backslash". The game draws
--- borders and separators as ordinary text, so a line with no letter and no
--- digit in it is decoration, and saying it is worse than saying nothing (P5).
--- @param s string
--- @return boolean
text.is_speakable = function(s) return s ~= "" and s:find("%w") ~= nil end

--- Prose the game wrote for a window, as a list of lines worth saying.
---
--- Long descriptions carry markup beyond colour -- <good>, <bad>, <dark> -- which
--- text.clean leaves alone because nothing else in the layer meets them, and they
--- would be read out as words. They are dropped here along with every other tag.
---
--- Split before cleaning, because the game's own line breaks are the only structure
--- the prose has: they separate a description from a list of what can be harvested
--- from it. Collapsing them first would leave one utterance several paragraphs long,
--- which is F5 with no way through it. Decoration lines go, per is_speakable.
--- @param s string|nil
--- @return string[]
text.lines = function(s)
  if not s or s == "" then return {} end

  local out = {}
  for line in (s .. "\n"):gmatch("([^\n]*)\n") do
    local plain = text.clean(line:gsub("<[^<>]*>", ""))
    if text.is_speakable(plain) then out[#out + 1] = plain end
  end
  return out
end

return text
