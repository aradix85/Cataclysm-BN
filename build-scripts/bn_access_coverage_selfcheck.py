"""Instrument check for coverage.py: does the source scan find every context the
data binds keys for? A category in data with no construction found in source means
the regex missed a construction shape, not that the screen does not exist."""

import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
CTX = re.compile(r'input_context\s*\w*\s*[({]\s*"([A-Z0-9_]+)"')
CTX_MEMBER = re.compile(r'\b\w*(?:ctxt|context)\w*\s*[({]\s*"([A-Z0-9_]+)"')
CTX_CATEGORY = re.compile(r'input_category\s*=\s*"([A-Z0-9_]+)"')
CTX_UNIQUE = re.compile(r'make_unique\s*<\s*input_context\s*>\s*\(\s*"([A-Z0-9_]+)"')
CTX_DEFAULT = re.compile(r'(?:ctxt|context)\w*\s*=\s*"([A-Z0-9_]+)"')

found = set()
for pattern in ("*.cpp", "*.h"):
    for path in (REPO / "src").rglob(pattern):
        text = path.read_text(encoding="utf-8", errors="replace")
        for regex in (CTX, CTX_MEMBER, CTX_CATEGORY, CTX_UNIQUE, CTX_DEFAULT):
            found |= set(regex.findall(text))

data = set()
for path in (REPO / "data" / "raw" / "keybindings").glob("*.json"):
    for entry in json.loads(path.read_text(encoding="utf-8")):
        if entry.get("type") == "keybinding":
            data.add(entry.get("category") or "?")

sys.stdout.reconfigure(encoding="utf-8")
print("in source, not in data:", len(found - data), sorted(found - data))
print("in data, not in source:", len(data - found), sorted(data - found))
print("source", len(found), "data", len(data), "union", len(found | data))
