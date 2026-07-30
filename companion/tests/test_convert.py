import json
import sqlite3
import zipfile
from pathlib import Path

import pytest

from inkcards import convert
from inkcards.convert import ConvertError, convert_apkg, convert_csv, html_to_text, write_deck_file
from inkcards.deckformat import Field, read_deck


# --- CSV --------------------------------------------------------------------


def _write(tmp_path: Path, name: str, text: str) -> Path:
    p = tmp_path / name
    p.write_text(text, encoding="utf-8")
    return p


def test_convert_csv_hsk_autodetect(tmp_path):
    src = _write(
        tmp_path,
        "hsk.csv",
        "hanzi,pinyin,english\n爱,ai4,to love\n八,ba1,eight\n",
    )
    deck = convert_csv(src, name="HSK 1")
    assert deck.name == "HSK 1"
    assert deck.has_pinyin()
    assert len(deck.cards) == 2
    assert deck.cards[0].get(Field.FRONT) == "爱"
    assert deck.cards[0].get(Field.PINYIN) == "ài"  # numbered pinyin normalised
    assert deck.cards[0].get(Field.BACK) == "to love"
    # Stable, 1-based ids.
    assert [c.card_id for c in deck.cards] == [1, 2]


def test_convert_csv_capitals_autodetect(tmp_path):
    src = _write(tmp_path, "caps.csv", "country,capital\nFrance,Paris\nJapan,Tokyo\n")
    deck = convert_csv(src)
    assert not deck.has_pinyin()
    assert deck.cards[0].get(Field.FRONT) == "France"
    assert deck.cards[0].get(Field.BACK) == "Paris"


def test_convert_csv_two_column_positional(tmp_path):
    src = _write(tmp_path, "q.csv", "prompt,response\nHola,Hello\n")
    deck = convert_csv(src)
    assert deck.cards[0].get(Field.FRONT) == "Hola"
    assert deck.cards[0].get(Field.BACK) == "Hello"


def test_convert_csv_explicit_columns_and_extras(tmp_path):
    src = _write(
        tmp_path,
        "x.csv",
        "q;a;ex;memo\nfoo;bar;an example;a note\n",
    )
    deck = convert_csv(
        src,
        front_col="q",
        back_col="a",
        example_col="ex",
        notes_col="memo",
        delimiter=";",
    )
    card = deck.cards[0]
    assert card.get(Field.FRONT) == "foo"
    assert card.get(Field.EXAMPLE) == "an example"
    assert card.get(Field.NOTES) == "a note"


def test_convert_csv_skips_blank_rows(tmp_path):
    src = _write(tmp_path, "b.csv", "front,back\na,1\n,\nb,2\n")
    deck = convert_csv(src)
    assert len(deck.cards) == 2


def test_convert_csv_unresolvable_columns(tmp_path):
    src = _write(tmp_path, "m.csv", "one,two,three\na,b,c\n")
    with pytest.raises(ConvertError):
        convert_csv(src)


def test_write_deck_file_roundtrip(tmp_path):
    src = _write(tmp_path, "caps.csv", "country,capital\nFrance,Paris\n")
    deck = convert_csv(src, name="Caps", font_hint="NotoSans")
    out = tmp_path / "out" / "caps.deck"
    write_deck_file(deck, out)
    assert out.exists()
    parsed = read_deck(out.read_bytes())
    assert parsed.name == "Caps"
    assert parsed.font_hint == "NotoSans"
    assert parsed.cards[0].get(Field.BACK) == "Paris"


# --- HTML helper ------------------------------------------------------------


def test_html_to_text():
    assert html_to_text("<b>bold</b> text") == "bold text"
    assert html_to_text("line1<br>line2") == "line1 line2"
    assert html_to_text("word [sound:audio.mp3]") == "word"
    assert html_to_text("a&nbsp;b &amp; c") == "a b & c"


# --- apkg -------------------------------------------------------------------


def _make_apkg(path: Path, models: dict, notes: list[tuple[int, int, str]], db_name="collection.anki2"):
    """Build a minimal .apkg readable by convert_apkg (schema-11 subset)."""
    db_path = path.parent / db_name
    conn = sqlite3.connect(db_path)
    conn.execute("CREATE TABLE col (id integer primary key, models text)")
    conn.execute("CREATE TABLE notes (id integer primary key, mid integer, flds text)")
    conn.execute("INSERT INTO col (id, models) VALUES (1, ?)", (json.dumps(models),))
    for note_id, mid, flds in notes:
        conn.execute("INSERT INTO notes (id, mid, flds) VALUES (?, ?, ?)", (note_id, mid, flds))
    conn.commit()
    conn.close()

    with zipfile.ZipFile(path, "w") as zf:
        zf.write(db_path, db_name)
        zf.writestr("media", "{}")
    db_path.unlink()


def test_convert_apkg_basic(tmp_path):
    models = {"100": {"name": "Basic", "flds": [{"name": "Front", "ord": 0}, {"name": "Back", "ord": 1}]}}
    notes = [
        (1111, 100, "Hello\x1f<b>Bonjour</b>"),
        (2222, 100, "Bye\x1fAu revoir"),
    ]
    apkg = tmp_path / "deck.apkg"
    _make_apkg(apkg, models, notes)

    deck = convert_apkg(apkg, name="French")
    assert deck.name == "French"
    assert len(deck.cards) == 2
    assert deck.cards[0].get(Field.FRONT) == "Hello"
    assert deck.cards[0].get(Field.BACK) == "Bonjour"  # HTML stripped
    # Notes are ordered by note id.
    assert deck.cards[1].get(Field.FRONT) == "Bye"


def test_convert_apkg_chinese_with_pinyin(tmp_path):
    models = {
        "200": {
            "name": "Chinese",
            "flds": [
                {"name": "Hanzi", "ord": 0},
                {"name": "Pinyin", "ord": 1},
                {"name": "English", "ord": 2},
            ],
        }
    }
    notes = [(1, 200, "你好\x1fni3 hao3\x1fhello")]
    apkg = tmp_path / "cn.apkg"
    _make_apkg(apkg, models, notes)

    deck = convert_apkg(apkg)
    card = deck.cards[0]
    assert card.get(Field.FRONT) == "你好"
    assert card.get(Field.PINYIN) == "nǐ hǎo"  # detected by field name + normalised
    assert card.get(Field.BACK) == "hello"
    assert deck.has_pinyin()


def test_convert_apkg_anki21_preferred(tmp_path):
    models = {"1": {"name": "Basic", "flds": [{"name": "Front", "ord": 0}, {"name": "Back", "ord": 1}]}}
    notes = [(1, 1, "a\x1fb")]
    apkg = tmp_path / "d.apkg"
    _make_apkg(apkg, models, notes, db_name="collection.anki21")
    deck = convert_apkg(apkg)
    assert deck.cards[0].get(Field.BACK) == "b"


def test_convert_apkg_zstd_unsupported(tmp_path):
    apkg = tmp_path / "new.apkg"
    with zipfile.ZipFile(apkg, "w") as zf:
        zf.writestr("collection.anki21b", b"\x28\xb5\x2f\xfd")  # zstd magic, unreadable here
        zf.writestr("media", "{}")
    with pytest.raises(ConvertError):
        convert_apkg(apkg)
