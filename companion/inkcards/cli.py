"""Command-line entry point: ``inkcards convert`` and ``inkcards stats``."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from . import __version__, stats
from .convert import ConvertError, convert_apkg, convert_csv, write_deck_file


def _detect_format(path: Path, explicit: str) -> str:
    if explicit != "auto":
        return explicit
    suffix = path.suffix.lower()
    if suffix == ".apkg":
        return "apkg"
    if suffix in (".csv", ".tsv"):
        return "csv"
    raise ConvertError(f"cannot infer format for {path}; pass --format csv|apkg")


def _cmd_convert(args: argparse.Namespace) -> int:
    src = Path(args.input)
    if not src.exists():
        print(f"error: input not found: {src}", file=sys.stderr)
        return 2
    try:
        fmt = _detect_format(src, args.format)
        if fmt == "csv":
            delimiter = "\t" if src.suffix.lower() == ".tsv" else args.delimiter
            deck = convert_csv(
                src,
                name=args.name,
                front_col=args.front_col,
                back_col=args.back_col,
                pinyin_col=args.pinyin_col,
                example_col=args.example_col,
                notes_col=args.notes_col,
                hint_col=args.hint_col,
                tags_col=args.tags_col,
                description=args.description,
                font_hint=args.font_hint,
                delimiter=delimiter,
            )
        else:
            deck = convert_apkg(
                src,
                name=args.name,
                front_field=args.front_field,
                back_field=args.back_field,
                pinyin_field=args.pinyin_field,
                description=args.description,
                font_hint=args.font_hint,
            )
    except ConvertError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

    if not deck.cards:
        print("error: no cards were produced from the input", file=sys.stderr)
        return 2

    out = Path(args.output) if args.output else src.with_suffix(".deck")
    write_deck_file(deck, out)
    print(f"Wrote {out} ({len(deck.cards)} cards, id {deck.deck_id})")
    return 0


def _cmd_stats(args: argparse.Namespace) -> int:
    today = stats.today_day(tz_offset_seconds=args.tz_offset)

    if args.deck:
        from .deckformat import read_deck, read_review_state

        deck = read_deck(Path(args.deck).read_bytes())
        state = None
        if args.state:
            state = read_review_state(Path(args.state).read_bytes())
        row = stats.compute_stats(deck, state, today, new_per_day=args.new_per_day)
        print(stats.format_stats_table([row]))
        return 0

    if not args.sd:
        print("error: pass --sd <path> or --deck <file>", file=sys.stderr)
        return 2

    rows = stats.gather_sd_stats(args.sd, today, new_per_day=args.new_per_day)
    print(stats.format_stats_table(rows))
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="inkcards", description="InkCards companion tools")
    parser.add_argument("--version", action="version", version=f"%(prog)s {__version__}")
    sub = parser.add_subparsers(dest="command", required=True)

    conv = sub.add_parser("convert", help="convert CSV or Anki .apkg into a .deck file")
    conv.add_argument("input", help="source file (.csv, .tsv or .apkg)")
    conv.add_argument("-o", "--output", help="output .deck path (default: input with .deck suffix)")
    conv.add_argument("--name", help="deck name (default: input file stem)")
    conv.add_argument("--description", default="", help="deck description")
    conv.add_argument("--font-hint", default="", help="suggested SD-card font family (e.g. NotoSansSC)")
    conv.add_argument("--format", choices=("auto", "csv", "apkg"), default="auto")
    # CSV column overrides.
    conv.add_argument("--front-col")
    conv.add_argument("--back-col")
    conv.add_argument("--pinyin-col")
    conv.add_argument("--example-col")
    conv.add_argument("--notes-col")
    conv.add_argument("--hint-col")
    conv.add_argument("--tags-col")
    conv.add_argument("--delimiter", default=",", help="CSV delimiter (default: comma)")
    # apkg field overrides.
    conv.add_argument("--front-field")
    conv.add_argument("--back-field")
    conv.add_argument("--pinyin-field")
    conv.set_defaults(func=_cmd_convert)

    st = sub.add_parser("stats", help="summarise review progress from an SD card")
    st.add_argument("--sd", help="path to the SD card root")
    st.add_argument("--deck", help="inspect a single .deck file instead of an SD card")
    st.add_argument("--state", help="a .rev file to pair with --deck")
    st.add_argument("--new-per-day", type=int, default=20, help="daily new-card limit for the figures")
    st.add_argument("--tz-offset", type=int, default=0, help="local timezone offset from UTC, in seconds")
    st.set_defaults(func=_cmd_stats)

    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":  # pragma: no cover
    raise SystemExit(main())
