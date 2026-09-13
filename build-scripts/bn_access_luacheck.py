#!/usr/bin/env python3
"""Judge an EmmyLua report, keeping only the accessibility layer.

The analyser walks the whole clone, so most of what it prints is upstream's Lua
and none of our business. One family inside data/access is permanent and is not
a defect:

    unresolved-require
        Every module the layer loads looks missing to the analyser. It resolves
        a module name the way a mod writes one -- dotted, `require("lib.x")` --
        against the workspace roots, while the layer has to write the path form
        `require("./lib/x")`: it is loaded by the fork rather than by the mod
        system, so it has no mod base path for the engine to resolve a dotted
        name against, and writing one makes the engine itself fail to load.

The hook-return family used to sit here too, and it was a real mismatch rather
than a fact of life: the generated annotation declared a HookResult return while
the engine also accepts a boolean veto or nothing at all. Widening that alias
removed every one of those findings, which is why the list is one family now.

They are counted rather than dropped. A filter that hides a family silently
hides the first genuine member of it too -- and a require that really is missing
reaches the player as silence, which is the one failure this project cannot
afford.

Anything outside that family is ours and is a finding until judged otherwise.
Exits 1 when there is one, so a caller needs no output parsing.

Reads the report from a file given as the first argument, or from stdin.
"""

import re
import sys

# Blocks open with "--- <path> [n warnings]"; findings carry their kind in
# square brackets at the end of the line.
BLOCK = re.compile(r"^---\s+(\S+)")
FINDING = re.compile(r"^\s*(?:warning|error|hint):")
KIND = re.compile(r"\[([a-z-]+)\]\s*$")

OURS = "data/access/"
KNOWN = {
    "unresolved-require",
}


def judge(lines):
    """Return (known count, list of findings to judge)."""
    ours = False
    known = 0
    found = []

    for line in lines:
        block = BLOCK.match(line)
        if block:
            ours = block.group(1).replace("\\", "/").startswith(OURS)
            continue
        if not ours or not FINDING.match(line):
            continue

        kind = KIND.search(line)
        if kind and kind.group(1) in KNOWN:
            known += 1
        else:
            found.append(line.strip())

    return known, found


def main():
    if len(sys.argv) > 1:
        with open(sys.argv[1], encoding="utf-8", errors="replace") as handle:
            lines = handle.readlines()
    else:
        lines = sys.stdin.readlines()

    known, found = judge(lines)

    print("EmmyLua, data/access only.")
    print(f"{known} known finding(s): the require resolver.")

    if not found:
        print("Nothing else. Clean.")
        return 0

    print(f"{len(found)} finding(s) to judge:")
    for line in found:
        print(f"  {line}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
