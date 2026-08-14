-- What was said, kept so it can be said again.
--
-- In NVDA sleep mode -- the mode this project requires -- NVDA's own review
-- cursor, repeat command and speech history are inert, so anything missed is
-- gone for good (P9). This is the layer's own scrollback: bounded, one entry
-- per keypress on the way back, and the reason speech is allowed to be lossy
-- at all.
--
-- Pure: a buffer is a plain table, passed in and out. No speech, no gapi, no
-- metatables -- which also keeps it storable should it ever need to survive a
-- save.

local scrollback = {}

--- @param limit integer|nil how many entries to keep, oldest dropped first
scrollback.new = function(limit) return { entries = {}, limit = limit or 100, cursor = 0 } end

--- Append a line, or count it as a repeat of the one before it.
---
--- A fight logs the same miss over and over. Storing each separately would push
--- everything else out of a bounded buffer, so consecutive repeats become one
--- entry with a count -- and the count is spoken, because "you miss" three
--- times over is different information from once.
scrollback.add = function(buffer, line)
  local last = buffer.entries[#buffer.entries]
  if last and last.text == line then
    last.count = last.count + 1
  else
    buffer.entries[#buffer.entries + 1] = { text = line, count = 1 }
    if #buffer.entries > buffer.limit then table.remove(buffer.entries, 1) end
  end
  -- A new message moves the reading position to it, so paging back always
  -- starts from what was just heard rather than from wherever it was left.
  buffer.cursor = #buffer.entries
end

--- The utterance for an entry, count included when it repeated.
local function utterance(entry)
  if not entry then return nil end
  if entry.count > 1 then return string.format("%s, %d times", entry.text, entry.count) end
  return entry.text
end

scrollback.current = function(buffer) return utterance(buffer.entries[buffer.cursor]) end

--- Step one entry towards the oldest. Returns nil at the end rather than
--- wrapping around silently: a wrap with no signal reads as a stuck key.
scrollback.older = function(buffer)
  if buffer.cursor <= 1 then return nil end
  buffer.cursor = buffer.cursor - 1
  return scrollback.current(buffer)
end

--- Step one entry towards the newest. Returns nil at the end, as above.
scrollback.newer = function(buffer)
  if buffer.cursor >= #buffer.entries then return nil end
  buffer.cursor = buffer.cursor + 1
  return scrollback.current(buffer)
end

scrollback.is_empty = function(buffer) return #buffer.entries == 0 end

return scrollback
