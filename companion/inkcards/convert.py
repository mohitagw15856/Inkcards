"""Convert source material (CSV, Anki ``.apkg``) into InkCards ``.deck`` files."""

from __future__ import annotations

import csv
import html
import json
import re
import sqlite3
import tempfile
import zipfile
from pathlib import Path

from . import pinyin
from .deckformat import Card, Deck, Field

# Header names the CSV importer recognises without an explicit mapping.
_FRONT_ALIASES = ("front", "hanzi", "question", "term", "word", "country")
_BACK_ALIASES = ("back", "english", "answer", "definition", "meaning", "capital")
_PINYIN_ALIASES = ("pinyin", "reading", "pronunciation")
_EXAMPLE_ALIASES = ("example", "sentence")
_NOTES_ALIASES = ("notes", "note")
_HINT_ALIASES = ("hint",)
_TAGS_ALIASES = ("tags", "tag")


class ConvertError(Exception):
    """Raised for user-facing conversion problems (bad input, unsupported file)."""


def _match_alias(headers: list[str], aliases: tuple[str, ...]) -> str | None:
    lowered = {h.lower(): h for h in headers}
    for alias in aliases:
        if alias in lowered:
            return lowered[alias]
    return None


def _add_field(card: Card, ftype: Field, value: str | None) -> None:
    if value is None:
        return
    value = value.strip()
    if value:
        card.set(ftype, value)


def deck_from_rows(
    rows: list[dict[str, str]],
    name: str,
    front_col: str,
    back_col: str,
    *,
    pinyin_col: str | None = None,
    example_col: str | None = None,
    notes_col: str | None = None,
    hint_col: str | None = None,
    tags_col: str | None = None,
    description: str = "",
    font_hint: str = "",
) -> Deck:
    """Build a :class:`Deck` from a list of row dictionaries."""
    deck = Deck(name=name, description=description, font_hint=font_hint)
    card_id = 1
    for row in rows:
        front = (row.get(front_col) or "").strip()
        back = (row.get(back_col) or "").strip()
        if not front and not back:
            continue
        card = Card(card_id=card_id)
        _add_field(card, Field.FRONT, front)
        if pinyin_col:
            raw = row.get(pinyin_col)
            if raw and raw.strip():
                card.set(Field.PINYIN, pinyin.normalise(raw))
        _add_field(card, Field.BACK, back)
        if example_col:
            _add_field(card, Field.EXAMPLE, row.get(example_col))
        if notes_col:
            _add_field(card, Field.NOTES, row.get(notes_col))
        if hint_col:
            _add_field(card, Field.HINT, row.get(hint_col))
        if tags_col:
            _add_field(card, Field.TAGS, row.get(tags_col))
        deck.cards.append(card)
        card_id += 1
    return deck


def convert_csv(
    path: str | Path,
    *,
    name: str | None = None,
    front_col: str | None = None,
    back_col: str | None = None,
    pinyin_col: str | None = None,
    example_col: str | None = None,
    notes_col: str | None = None,
    hint_col: str | None = None,
    tags_col: str | None = None,
    description: str = "",
    font_hint: str = "",
    delimiter: str = ",",
) -> Deck:
    """Convert a CSV file to a :class:`Deck`.

    Columns are auto-detected from common header names when not given explicitly.
    A two-column file with unrecognised headers is treated as front, back.
    """
    path = Path(path)
    with path.open("r", encoding="utf-8-sig", newline="") as fh:
        reader = csv.DictReader(fh, delimiter=delimiter)
        if reader.fieldnames is None:
            raise ConvertError(f"{path}: empty CSV (no header row)")
        headers = list(reader.fieldnames)
        rows = [dict(r) for r in reader]

    front = front_col or _match_alias(headers, _FRONT_ALIASES)
    back = back_col or _match_alias(headers, _BACK_ALIASES)
    if (front is None or back is None) and len(headers) == 2:
        # Fall back to positional mapping for a plain two-column file.
        front = front or headers[0]
        back = back or headers[1]
    if front is None or back is None:
        raise ConvertError(
            f"{path}: could not determine front/back columns from headers {headers}; "
            "pass --front-col and --back-col"
        )

    return deck_from_rows(
        rows,
        name=name or path.stem,
        front_col=front,
        back_col=back,
        pinyin_col=pinyin_col or _match_alias(headers, _PINYIN_ALIASES),
        example_col=example_col or _match_alias(headers, _EXAMPLE_ALIASES),
        notes_col=notes_col or _match_alias(headers, _NOTES_ALIASES),
        hint_col=hint_col or _match_alias(headers, _HINT_ALIASES),
        tags_col=tags_col or _match_alias(headers, _TAGS_ALIASES),
        description=description,
        font_hint=font_hint,
    )


_TAG_RE = re.compile(r"<[^>]+>")
_SOUND_RE = re.compile(r"\[sound:[^\]]*\]")
_WS_RE = re.compile(r"\s+")


def html_to_text(value: str) -> str:
    """Reduce an Anki HTML field to plain text."""
    value = _SOUND_RE.sub("", value)
    value = value.replace("<br>", " ").replace("<br/>", " ").replace("<br />", " ")
    value = _TAG_RE.sub("", value)
    value = html.unescape(value)
    value = value.replace("\xa0", " ")
    return _WS_RE.sub(" ", value).strip()


def _open_anki_collection(zf: zipfile.ZipFile, tmpdir: Path) -> Path:
    names = set(zf.namelist())
    if "collection.anki21b" in names and "collection.anki21" not in names and "collection.anki2" not in names:
        raise ConvertError(
            "this .apkg uses the newest zstd-compressed schema (collection.anki21b). "
            "Re-export from Anki with 'Support older Anki versions' enabled to get a "
            "readable collection."
        )
    for candidate in ("collection.anki21", "collection.anki2"):
        if candidate in names:
            out = tmpdir / candidate
            out.write_bytes(zf.read(candidate))
            return out
    raise ConvertError("no collection database found inside the .apkg")


def convert_apkg(
    path: str | Path,
    *,
    name: str | None = None,
    front_field: str | None = None,
    back_field: str | None = None,
    pinyin_field: str | None = None,
    description: str = "",
    font_hint: str = "",
) -> Deck:
    """Convert an Anki ``.apkg`` export to a :class:`Deck`.

    Fields are mapped by note-type field name. Without overrides the first field
    becomes the front, the second the back, and a field named like "Pinyin" or
    "Reading" becomes the pinyin field.
    """
    path = Path(path)
    with tempfile.TemporaryDirectory() as td:
        tmpdir = Path(td)
        with zipfile.ZipFile(path) as zf:
            db_path = _open_anki_collection(zf, tmpdir)

        conn = sqlite3.connect(db_path)
        try:
            models_json = conn.execute("SELECT models FROM col").fetchone()[0]
            models = json.loads(models_json)
            # mid -> ordered list of field names.
            model_fields: dict[str, list[str]] = {}
            for mid, model in models.items():
                flds = sorted(model["flds"], key=lambda f: f["ord"])
                model_fields[str(mid)] = [f["name"] for f in flds]

            note_rows = conn.execute("SELECT id, mid, flds FROM notes ORDER BY id").fetchall()
        finally:
            conn.close()

    deck = Deck(name=name or path.stem, description=description, font_hint=font_hint)
    card_id = 1
    for _note_id, mid, flds in note_rows:
        field_names = model_fields.get(str(mid), [])
        values = flds.split("\x1f")
        by_name = {
            field_names[i].lower(): html_to_text(values[i])
            for i in range(min(len(field_names), len(values)))
        }

        def pick(explicit: str | None, aliases: tuple[str, ...], default_ord: int | None) -> str:
            if explicit and explicit.lower() in by_name:
                return by_name[explicit.lower()]
            for alias in aliases:
                if alias in by_name:
                    return by_name[alias]
            if default_ord is not None and default_ord < len(values):
                return html_to_text(values[default_ord])
            return ""

        front = pick(front_field, _FRONT_ALIASES, 0)
        back = pick(back_field, _BACK_ALIASES, 1)
        pin = pick(pinyin_field, _PINYIN_ALIASES, None)
        if not front and not back:
            continue

        card = Card(card_id=card_id)
        _add_field(card, Field.FRONT, front)
        if pin:
            card.set(Field.PINYIN, pinyin.normalise(pin))
        _add_field(card, Field.BACK, back)
        deck.cards.append(card)
        card_id += 1

    return deck


def write_deck_file(deck: Deck, out_path: str | Path) -> None:
    from .deckformat import write_deck

    out_path = Path(out_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(write_deck(deck))
