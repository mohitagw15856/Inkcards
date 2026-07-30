#!/usr/bin/env python3
"""Build the sample .deck files from their CSV sources using the companion tool.

Run from anywhere:

    python3 decks/build.py

This regenerates decks/*.deck from decks/src/*.csv. It imports the companion
package, so install it first (``pip install -e companion``) or run with the
companion directory on PYTHONPATH.
"""

from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent

# Allow running without installing the companion by adding it to the path.
sys.path.insert(0, str(REPO / "companion"))

from inkcards.convert import convert_csv, write_deck_file  # noqa: E402

SRC = HERE / "src"

BUILDS = [
    dict(
        csv="hsk1.csv",
        out="hsk1.deck",
        name="HSK 1 Vocabulary",
        description="The 150-word HSK 1 Mandarin vocabulary list.",
        font_hint="NotoSansSC",
    ),
    dict(
        csv="world-capitals.csv",
        out="world-capitals.deck",
        name="Capitals of the World",
        description="Sovereign countries and their capital cities.",
        font_hint="",
    ),
    dict(
        csv="demo.csv",
        out="demo.deck",
        name="InkCards Demo",
        description="The demo deck shown in the README.",
        font_hint="",
    ),
]


def main() -> int:
    for build in BUILDS:
        deck = convert_csv(
            SRC / build["csv"],
            name=build["name"],
            description=build["description"],
            font_hint=build["font_hint"],
        )
        out = HERE / build["out"]
        write_deck_file(deck, out)
        print(f"{build['csv']:24s} -> {build['out']:22s} {len(deck.cards):4d} cards  id {deck.deck_id}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
