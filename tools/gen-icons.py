#!/usr/bin/env python3
"""Generate the light-theme icon set from the dark-theme sources.

TagOS icons are single-colour line art. Qt's SVG renderer has no `currentColor`
support, so instead of tinting at runtime (which needs shader effects and breaks
on the software backend used by the smoke test) we ship two static sets:

    themes/icons/*.svg        stroke #F4F4F5   (dark theme)
    themes/icons/light/*.svg  stroke #14161B   (light theme)

Run this after editing any icon:  python3 tools/gen-icons.py
"""
from __future__ import annotations

import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[1]
SRC = ROOT / "themes/icons"
DST = SRC / "light"

DARK_STROKE = "#F4F4F5"
LIGHT_STROKE = "#14161B"
# The brand mark keeps its accent colour in both themes.
SKIP = {"tagos-logo.svg"}


def convert(text: str) -> str:
    text = text.replace(f'stroke="{DARK_STROKE}"', f'stroke="{LIGHT_STROKE}"')
    text = text.replace(f'fill="{DARK_STROKE}"', f'fill="{LIGHT_STROKE}"')
    # Older icons used the previous text colour.
    text = text.replace('stroke="#F5EDE6"', f'stroke="{LIGHT_STROKE}"')
    text = text.replace('fill="#F5EDE6"', f'fill="{LIGHT_STROKE}"')
    return text


def main() -> int:
    DST.mkdir(parents=True, exist_ok=True)
    written = 0
    for path in sorted(SRC.glob("*.svg")):
        if path.name in SKIP:
            continue
        target = DST / path.name
        target.write_text(convert(path.read_text(encoding="utf-8")), encoding="utf-8", newline="\n")
        written += 1
    print(f"gen-icons: wrote {written} light icons to {DST.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
