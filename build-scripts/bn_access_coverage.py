"""Measure the accessibility surface of the game from the source.

Every number this project quotes about how much work is left has to be
re-derivable, because upstream changes daily. This script is that derivation:
run it, read werk/logs/coverage.md, and quote nothing that is not in there.

Counted from:
  src/*.cpp, src/*.h            input contexts, uilist use, hook firing points,
                                message and prompt call sites
  data/raw/keybindings/*.json   the commands each context binds, and the whole
                                verb list the player meets in the field
  src/handle_action.cpp         what each verb does: speak, ask, or open a screen

Usage: python build-scripts/bn_access_coverage.py            measure, write the report, print the drift
       python build-scripts/bn_access_coverage.py --record   the same, and accept the result as the
                                                             new baseline (commit it)
       --out PATH      where the report goes; defaults to `out/coverage.md` in the clone
       --baseline PATH which baseline to judge against; defaults to the one beside this script

This lives in the clone rather than beside it because CI runs it: a firing point
of ours that upstream's next refactor deletes still compiles and still passes
every test, and the drift is the only thing that sees it go. Local runs go
through `werk\\coverage.py`, which is a wrapper passing this machine's paths.

The baseline is the point of this: after merging upstream, the drift says which
screens and which verbs the game gained, lost or resized while nobody was
looking. Either kind that appears there is one the layer does not speak yet.
"""

import argparse
import json
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path

# build-scripts/ sits at the root of the clone, so the repository is one up.
REPO = Path(__file__).resolve().parents[1]
DEFAULT_OUT = REPO / "out" / "coverage.md"
DEFAULT_BASELINE = Path(__file__).resolve().parent / "bn_access_coverage_baseline.json"

# The one context our layer registers its actions in today.
REACHED = {"DEFAULTMODE"}

CTX = re.compile(r'input_context\s*\w*\s*[({]\s*"([A-Z0-9_]+)"')
# The inventory, the vehicle screens and the main menu build their context in a
# constructor initialiser list, where the type name is on another line entirely.
# Missing those cost the first count of this file a third of the game's screens.
CTX_MEMBER = re.compile(r'\b\w*(?:ctxt|context)\w*\s*[({]\s*"([A-Z0-9_]+)"')
# Three more shapes: a uilist naming its own category, a context built through
# make_unique, and a category arriving as a default argument.
CTX_CATEGORY = re.compile(r'input_category\s*=\s*"([A-Z0-9_]+)"')
CTX_UNIQUE = re.compile(r'make_unique\s*<\s*input_context\s*>\s*\(\s*"([A-Z0-9_]+)"')
CTX_DEFAULT = re.compile(r'(?:ctxt|context)\w*\s*=\s*"([A-Z0-9_]+)"')
REGISTER = re.compile(r'\.register_action\s*\(\s*"([A-Z0-9_]+)"')
# Where a function is defined, so a verb can be followed one level down into the
# file that owns the screen it opens. Definition-ish: a line that opens a body
# rather than ending in a semicolon.
DEFN = re.compile(r"^[A-Za-z_][\w:<>,*&\s]*?\b(?:(\w+)::)?(\w+)\s*\([^;]*$")
UILIST = re.compile(r"\builist\b")
UILIST_DECL = re.compile(r"\builist\s+\w+\s*[;({]")
HOOK = re.compile(r'run_hooks\s*\(\s*"(\w+)"')
ADD_MSG = re.compile(r"\badd_msg\w*\s*\(")
POPUP = re.compile(r"\b(query_yn|query_popup|popup_getkey|query_int)\s*\(")
CALL = re.compile(r"\b([a-z_][a-z0-9_]*(?:::[a-z_][a-z0-9_]*)*)\s*\(")


def sources():
    for pattern in ("*.cpp", "*.h"):
        for path in sorted((REPO / "src").rglob(pattern)):
            yield path


def scan():
    """One pass over the source, attributing registrations to the context above them."""
    contexts = defaultdict(lambda: {"files": Counter(), "constructed": 0, "registered": Counter()})
    uilist_files = Counter()
    uilist_total = 0
    uilist_decls = 0
    hooks = Counter()
    hook_files = defaultdict(set)
    defs = {}
    file_calls = defaultdict(set)
    add_msg = 0
    popups = 0

    for path in sources():
        rel = path.relative_to(REPO).as_posix()
        current = None
        for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
            found = (
                CTX.search(line)
                or CTX_MEMBER.search(line)
                or CTX_CATEGORY.search(line)
                or CTX_UNIQUE.search(line)
                or CTX_DEFAULT.search(line)
            )
            if found:
                current = found.group(1)
                entry = contexts[current]
                entry["files"][rel] += 1
                entry["constructed"] += 1
            registered = REGISTER.search(line)
            if registered and current:
                contexts[current]["registered"][registered.group(1)] += 1
            uses = len(UILIST.findall(line))
            if uses:
                uilist_total += uses
                uilist_files[rel] += uses
            uilist_decls += len(UILIST_DECL.findall(line))
            hook = HOOK.search(line)
            if hook:
                hooks[hook.group(1)] += 1
                hook_files[hook.group(1)].add(rel)
            defined = DEFN.match(line)
            if defined and rel.endswith(".cpp"):
                cls, name = defined.group(1), defined.group(2)
                defs.setdefault(name, []).append(rel)
                if cls:
                    defs.setdefault(f"{cls}::{name}", []).append(rel)
            add_msg += len(ADD_MSG.findall(line))
            popups += len(POPUP.findall(line))
            for called in CALL.findall(line):
                file_calls[rel].add(called)

    return {
        "contexts": contexts,
        "uilist_files": uilist_files,
        "uilist_total": uilist_total,
        "uilist_decls": uilist_decls,
        "hooks": hooks,
        "hook_files": hook_files,
        "defs": defs,
        "file_calls": file_calls,
        "add_msg": add_msg,
        "popups": popups,
    }


def bindings():
    """How many commands each context binds in data, and how many keys they take."""
    per_context = Counter()
    keys = Counter()
    for path in sorted((REPO / "data" / "raw" / "keybindings").glob("*.json")):
        entries = json.loads(path.read_text(encoding="utf-8"))
        for entry in entries:
            if entry.get("type") != "keybinding":
                continue
            category = entry.get("category", "?")
            per_context[category] += 1
            keys[category] += len(entry.get("bindings", []))
    return per_context, keys


def screen_behind(call, defs, screens, file_calls, depth=2):
    """The screen file a call ends at, following wrappers a couple of levels down.

    A verb rarely opens a screen itself: `eat` calls `avatar_action::eat`, which
    calls a menu in `game_inventory.cpp`, which builds the selector that owns the
    context in `inventory_ui.cpp`. Stopping at the first hop reported those verbs
    as doing nothing at all.

    **Bare names collide across a tree this size**, so only the first hop may use
    one; deeper hops follow qualified names only. Without that, three hops of
    common helper names connect almost any verb to almost any screen, and the
    answer looks precise while being noise.
    """
    seen = set()
    frontier = [(call, 0)]
    while frontier:
        name, level = frontier.pop(0)
        if level > depth:
            continue
        found = defs.get(name)
        if found is None and level == 0:
            found = defs.get(name.split("::")[-1])
        for where in found or []:
            if where in screens:
                return where, level
            if where not in seen:
                seen.add(where)
                frontier.extend(
                    (c, level + 1) for c in file_calls.get(where, ()) if "::" in c
                )
    return None, None


def verbs(defs, uilist_files, context_files, file_calls):
    """The player's whole verb list in the field, and what each one does.

    `action_ident` in src/action.cpp maps every ACTION_* to the string the data
    binds keys to, so the two sides can be joined. The case body in
    handle_action.cpp then says, in signals rather than verdicts, whether the
    verb answers with a message, asks a question, or opens something.
    """
    ident = dict(
        (m.group(2), m.group(1))
        for m in re.finditer(
            r"case\s+(ACTION_[A-Z0-9_]+):\s*\n\s*return\s+\"([^\"]+)\";",
            (REPO / "src" / "action.cpp").read_text(encoding="utf-8", errors="replace"),
        )
    )

    handler = (REPO / "src" / "handle_action.cpp").read_text(encoding="utf-8", errors="replace")
    blocks = {}
    cases = list(re.finditer(r"case\s+(ACTION_[A-Z0-9_]+):", handler))
    for i, case in enumerate(cases):
        end = cases[i + 1].start() if i + 1 < len(cases) else len(handler)
        blocks[case.group(1)] = handler[case.end(): end]
    # C++ fall-through: a label with an empty body runs the next label's code, so
    # the whole group shares it. Without this every verb in a group but the last
    # looks like it does nothing at all, which is how the eight view-shifting
    # verbs first read as unexplained.
    empty = re.compile(r"^\s*(//[^\n]*\s*)*$")
    for i, case in enumerate(cases):
        name = case.group(1)
        if empty.match(blocks[name]):
            for later in cases[i + 1:]:
                if not empty.match(blocks[later.group(1)]):
                    blocks[name] = blocks[later.group(1)]
                    break

    bound = {}
    for path in sorted((REPO / "data" / "raw" / "keybindings").glob("*.json")):
        for entry in json.loads(path.read_text(encoding="utf-8")):
            if entry.get("type") == "keybinding" and entry.get("category") == "DEFAULTMODE":
                bound[entry["id"]] = entry.get("name", "")

    rows = []
    for verb, name in sorted(bound.items()):
        action = ident.get(verb, "")
        body = blocks.get(action, "")
        signals = []
        if verb.startswith("bn_access_"):
            signals.append("ours")
        if re.search(r"\badd_msg\w*\s*\(", body):
            signals.append("speaks")
        if re.search(r"\b(query_yn|query_popup|query_int)\s*\(", body):
            signals.append("asks")
        calls = [c for c in re.findall(r"\b([a-z_][a-z0-9_]*(?:::[a-z_][a-z0-9_]*)?)\s*\(", body)
                 if c not in {"if", "for", "while", "switch", "return", "sizeof", "_"}]
        # One level down: the file that owns what the verb calls says whether the
        # verb opens a screen, and which one. A bare name collides across the
        # tree, so the qualified name is tried first and a screen file wins over
        # an unrelated namesake.
        opens = []
        screens = set(uilist_files) | set(context_files)
        for call in dict.fromkeys(calls):
            found, level = screen_behind(call, defs, screens, file_calls)
            if found and level == 0:
                opens.append(found)
            elif found:
                # Deeper than one hop the file name is a guess, but the wrapper
                # that leads there is not: name it so a session knows where to read.
                opens.append(f"via {call}")
        if opens:
            signals.append("opens a screen")
        rows.append((
            verb, name, action, ", ".join(signals),
            ", ".join(dict.fromkeys(opens))[:60] or ", ".join(dict.fromkeys(calls))[:60],
        ))
    return rows


def snapshot(contexts, bound, hooks, verb_rows):
    """What is worth comparing across an upstream merge: screens, hooks, verbs."""
    return {
        "contexts": {
            name: {"bound": bound.get(name, 0), "registered": len(entry["registered"])}
            for name, entry in sorted(contexts.items())
        },
        "hooks": dict(sorted(hooks.items())),
        "verbs": {row[0]: row[3] for row in verb_rows},
    }


def drift(now, before):
    """Lines describing what changed, or an empty list when nothing did."""
    if not before:
        return ["No baseline recorded yet. Run with --record to make this one the baseline."]

    lines = []
    old_ctx, new_ctx = before.get("contexts", {}), now["contexts"]
    for name in sorted(set(new_ctx) - set(old_ctx)):
        lines.append(f"NEW SCREEN {name}: {new_ctx[name]['bound']} commands bound, not spoken yet")
    for name in sorted(set(old_ctx) - set(new_ctx)):
        lines.append(f"GONE {name}")
    for name in sorted(set(old_ctx) & set(new_ctx)):
        was, is_now = old_ctx[name]["bound"], new_ctx[name]["bound"]
        if was != is_now:
            lines.append(f"RESIZED {name}: {was} -> {is_now} commands bound")

    old_hooks, new_hooks = before.get("hooks", {}), now["hooks"]
    for name in sorted(set(new_hooks) - set(old_hooks)):
        lines.append(f"NEW HOOK {name}, {new_hooks[name]} firing points")
    for name in sorted(set(old_hooks) - set(new_hooks)):
        lines.append(f"HOOK GONE {name}")

    old_verbs, new_verbs = before.get("verbs", {}), now["verbs"]
    for name in sorted(set(new_verbs) - set(old_verbs)):
        lines.append(f"NEW VERB {name}: signals [{new_verbs[name]}], nothing speaks it yet")
    for name in sorted(set(old_verbs) - set(new_verbs)):
        lines.append(f"VERB GONE {name}")
    for name in sorted(set(old_verbs) & set(new_verbs)):
        if old_verbs[name] != new_verbs[name]:
            lines.append(f"VERB CHANGED {name}: [{old_verbs[name]}] -> [{new_verbs[name]}]")

    return lines or ["No change against the baseline."]


def table(rows, headers):
    out = ["| " + " | ".join(headers) + " |", "|" + "---|" * len(headers)]
    for row in rows:
        out.append("| " + " | ".join(str(cell) for cell in row) + " |")
    return "\n".join(out)


def main():
    parser = argparse.ArgumentParser(description="Measure the accessibility surface of the game.")
    parser.add_argument("--record", action="store_true",
                        help="accept this result as the new baseline")
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT, help="where to write the report")
    parser.add_argument("--baseline", type=Path, default=DEFAULT_BASELINE,
                        help="which baseline to judge against")
    args = parser.parse_args()
    record, out, baseline = args.record, args.out, args.baseline
    data = scan()
    bound, keys = bindings()
    verb_rows = verbs(
        data["defs"],
        data["uilist_files"],
        {f for entry in data["contexts"].values() for f in entry["files"]},
        data["file_calls"],
    )
    contexts = data["contexts"]

    now = snapshot(contexts, bound, data["hooks"], verb_rows)
    before = json.loads(baseline.read_text(encoding="utf-8")) if baseline.exists() else None
    changes = drift(now, before)

    ranked = sorted(
        contexts.items(),
        key=lambda item: (bound.get(item[0], 0), len(item[1]["registered"])),
        reverse=True,
    )

    # What one piece of work reaches: a context whose file builds uilists is
    # partly or wholly served by making that one class speak.
    in_uilist_file = [
        name
        for name, entry in contexts.items()
        if any(data["uilist_files"].get(f) for f in entry["files"])
    ]
    uilist_bound = sum(bound.get(name, 0) for name in in_uilist_file)
    silent_screens = [name for name in contexts if not bound.get(name)]

    lines = []
    lines.append("# Accessibility coverage, generated by `werk\\coverage.py`")
    lines.append("")
    lines.append("Do not edit. Re-run the script; every number here is derived from the source.")
    lines.append("")
    lines.append("## The size of the job")
    lines.append("")
    lines.append(
        table(
            [
                ["Input contexts, distinct", len(contexts)],
                ["Input contexts our layer reaches", len([c for c in contexts if c in REACHED])],
                ["Contexts with commands bound in data", len([c for c in contexts if bound.get(c)])],
                ["Commands bound across all contexts", sum(bound.values())],
                ["Verbs the player has in the field", len(verb_rows)],
                ["Verbs whose case body already speaks or asks", sum(
                    1 for r in verb_rows if "speaks" in r[3] or "asks" in r[3])],
                ["Verbs that open a screen", sum(
                    1 for r in verb_rows if "opens a screen" in r[3])],
                ["Verbs with no signal at all", sum(1 for r in verb_rows if not r[3])],
                ["Contexts in a file that builds uilists", len(in_uilist_file)],
                ["Commands bound in those contexts", uilist_bound],
                ["Contexts binding no commands in data", len(silent_screens)],
                ["uilist mentions in source", data["uilist_total"]],
                ["uilist instances declared", data["uilist_decls"]],
                ["Files mentioning uilist", len(data["uilist_files"])],
                ["Message call sites (one hook covers all)", data["add_msg"]],
                ["Blocking prompt call sites (one hook covers all)", data["popups"]],
                ["Hook firing points in source", sum(data["hooks"].values())],
                ["Distinct hooks fired", len(data["hooks"])],
            ],
            ["What", "Count"],
        )
    )
    lines.append("")

    lines.append("## What changed since the recorded baseline")
    lines.append("")
    lines.append(
        f"The baseline is `{baseline.name}` beside the script, and it is committed. Re-run this"
        " script after merging upstream: a screen that appears here is one the layer does not"
        " speak yet. Accept a reviewed result with `--record`."
    )
    lines.append("")
    for change in changes:
        lines.append(f"- {change}")
    lines.append("")

    lines.append("## Every verb the player has in the field")
    lines.append("")
    lines.append(
        "The 128-odd commands of `DEFAULTMODE` are the game as the player meets it: this is the"
        " list a milestone is checked against, not the screen list. `Signals` are read from the"
        " verb's case body in `handle_action.cpp` and are hints, not verdicts — `speaks` means the"
        " body adds a message and therefore already reaches the layer, `asks` means it raises a"
        " prompt, `uilist` means it builds a menu and so is answered by the menu step. A verb with"
        " no signal at all is one to open by hand and look at. `Calls` is where to start reading."
    )
    lines.append("")
    lines.append(table(verb_rows, ["Verb", "Name", "Action", "Signals", "Calls"]))
    lines.append("")

    lines.append("## Every input context, largest first")
    lines.append("")
    lines.append(
        "`Bound` is how many commands the context declares in `data/raw/keybindings/`, which is the"
        " size of the screen as the player meets it. `Registered` counts distinct `register_action`"
        " calls attributed to the context in C++. `uilist` marks a context whose file also builds"
        " uilists, so making uilist speak reaches part of it."
    )
    lines.append("")
    rows = []
    for name, entry in ranked:
        files = ", ".join(sorted(entry["files"]))
        uses_uilist = any(data["uilist_files"].get(f) for f in entry["files"])
        rows.append(
            [
                name,
                bound.get(name, 0),
                len(entry["registered"]),
                "yes" if uses_uilist else "",
                "REACHED" if name in REACHED else "",
                files if len(files) < 90 else files[:87] + "...",
            ]
        )
    lines.append(table(rows, ["Context", "Bound", "Registered", "uilist", "Layer", "Where"]))
    lines.append("")

    lines.append("## Where uilist is built, most first")
    lines.append("")
    rows = [[f, n] for f, n in data["uilist_files"].most_common(25)]
    lines.append(table(rows, ["File", "Mentions"]))
    lines.append("")

    lines.append("## Hooks that already exist, and where they fire")
    lines.append("")
    rows = [
        [name, count, ", ".join(sorted(data["hook_files"][name]))]
        for name, count in sorted(data["hooks"].items(), key=lambda kv: -kv[1])
    ]
    lines.append(table(rows, ["Hook", "Firing points", "Files"]))
    lines.append("")

    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(lines), encoding="utf-8")

    if record:
        baseline.write_text(
            json.dumps(now, indent=2, sort_keys=True, ensure_ascii=False) + "\n", encoding="utf-8"
        )

    reached = len([c for c in contexts if c in REACHED])
    print(f"contexts={len(contexts)} reached={reached} bound_commands={sum(bound.values())}")
    print(f"uilist_mentions={data['uilist_total']} uilist_files={len(data['uilist_files'])}")
    print(f"add_msg_sites={data['add_msg']} prompt_sites={data['popups']}")
    print(f"hooks={len(data['hooks'])} firing_points={sum(data['hooks'].values())}")
    print(f"written {out}")
    print("drift against baseline:")
    for change in changes:
        print(f"  {change}")
    if record:
        print(f"baseline recorded at {baseline}")


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    main()
