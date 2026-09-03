#!/usr/bin/env python3
"""What a downloaded build needs in order to speak, asserted against the package.

The failure this exists for is silent. src/tts.cpp loads nvdaControllerClient.dll
by name and treats a miss as silence rather than as a crash, and the options that
turn off tiles, music and animations are equally quiet when absent: the game starts,
plays, and says nothing at all. Nobody reading a green build would know, and the
player who downloaded it has no way to tell a missing file from a broken layer.

So the package is read rather than trusted. Run against the directory the Windows
job installs and uploads, or against any unpacked build:

    python3 build-scripts/bn_access_package_check.py cataclysmbn-experimental
"""

import json
import struct
import sys
from pathlib import Path

# Set in build-data/bn-access/options.json and installed into config/. Named here
# as well so that quietly dropping one is a failure rather than a diff nobody reads.
REQUIRED_OPTIONS = {
    "USE_LANG",
    "USE_TILES",
    "PIXEL_MINIMAP",
    "ANIMATIONS",
    "ANIMATION_DELAY",
    "MUSIC_VOLUME",
}

PE_MACHINE_AMD64 = 0x8664


def pe_machine(path: Path) -> int:
    """The architecture a PE file was built for, read out of its header.

    Checked because the wrong architecture fails exactly like a missing file:
    LoadLibraryW returns null, tts falls back to silence, and nothing is logged.
    A 32-bit copy of this library is easy to pick up by accident -- it is what the
    NVDA source tree ships alongside the 64-bit one.
    """
    raw = path.read_bytes()
    pe_offset = struct.unpack_from("<I", raw, 0x3C)[0]
    if raw[pe_offset:pe_offset + 4] != b"PE\0\0":
        raise ValueError(f"{path.name} is not a PE image")
    return struct.unpack_from("<H", raw, pe_offset + 4)[0]


def check(root: Path) -> list[str]:
    """Everything wrong with this package, as plain sentences."""
    wrong: list[str] = []

    library = root / "nvdaControllerClient.dll"
    if not library.is_file():
        wrong.append(
            "nvdaControllerClient.dll is missing from the package root, so the "
            "build starts and says nothing"
        )
    else:
        machine = pe_machine(library)
        if machine != PE_MACHINE_AMD64:
            wrong.append(
                f"nvdaControllerClient.dll is built for 0x{machine:x} rather than "
                "x64, so the game cannot load it"
            )

    if not (root / "license_nvda_controllerclient.txt").is_file():
        wrong.append("the LGPL licence for the bundled speech library is missing")

    return wrong


def check_options(root: Path) -> list[str]:
    """The settings a first run needs before anyone can reach the settings screen."""
    wrong: list[str] = []

    options = root / "config" / "options.json"
    if not options.is_file():
        wrong.append(
            "config/options.json is missing, so a first run opens on a language "
            "picker that draws its own window and speaks nothing"
        )
        return wrong

    try:
        entries = json.loads(options.read_text(encoding="utf-8"))
    except json.JSONDecodeError as bad:
        wrong.append(f"config/options.json is not readable as JSON: {bad}")
        return wrong

    named = {entry.get("name") for entry in entries if isinstance(entry, dict)}
    for missing in sorted(REQUIRED_OPTIONS - named):
        wrong.append(f"config/options.json no longer sets {missing}")

    return wrong


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(__doc__)
        return 2

    root = Path(argv[1])
    if not root.is_dir():
        print(f"No such package directory: {root}")
        return 2

    wrong = check(root) + check_options(root)
    if wrong:
        print(f"The package cannot speak. {len(wrong)} problem(s):")
        for line in wrong:
            print(f"  - {line}")
        return 1

    print("The package carries its speech library and its first-run settings.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
