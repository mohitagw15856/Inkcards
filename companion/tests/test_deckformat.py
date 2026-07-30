import struct

import pytest

from inkcards import deckformat
from inkcards.deckformat import (
    Card,
    Deck,
    Field,
    ReviewRecord,
    ReviewState,
    deck_id,
    fnv1a32,
    read_deck,
    read_review_state,
    write_deck,
    write_review_state,
)


def test_fnv1a32_known_vectors():
    # Canonical FNV-1a 32-bit test vectors (must match the C++ core).
    assert fnv1a32(b"") == 0x811C9DC5
    assert fnv1a32(b"a") == 0xE40C292C
    assert fnv1a32(b"foobar") == 0xBF9CF968
    assert deck_id("a") == "e40c292c"


def test_varint_roundtrip():
    for value in (0, 1, 127, 128, 300, 16384, 0xFFFFFFFF):
        buf = bytearray()
        deckformat.write_varint(buf, value)
        got, pos = deckformat.read_varint(bytes(buf), 0)
        assert got == value
        assert pos == len(buf)


def test_string_roundtrip_unicode():
    for text in ("", "hello", "你好", "中文 mixed"):
        buf = bytearray()
        deckformat.write_string(buf, text)
        got, pos = deckformat.read_string(bytes(buf), 0)
        assert got == text
        assert pos == len(buf)


def _sample_deck() -> Deck:
    deck = Deck(name="Demo Deck", description="a description", font_hint="NotoSansSC")
    c1 = Card(card_id=1)
    c1.set(Field.FRONT, "你好")
    c1.set(Field.PINYIN, "nǐ hǎo")
    c1.set(Field.BACK, "hello")
    c2 = Card(card_id=2)
    c2.set(Field.FRONT, "France")
    c2.set(Field.BACK, "Paris")
    deck.cards = [c1, c2]
    return deck


def test_deck_roundtrip():
    deck = _sample_deck()
    data = write_deck(deck)
    assert data[:4] == b"INKD"
    assert data[5] & deckformat.DECK_FLAG_HAS_PINYIN  # pinyin flag set

    parsed = read_deck(data)
    assert parsed.name == "Demo Deck"
    assert parsed.font_hint == "NotoSansSC"
    assert len(parsed.cards) == 2
    assert parsed.cards[0].get(Field.FRONT) == "你好"
    assert parsed.cards[0].get(Field.PINYIN) == "nǐ hǎo"
    assert parsed.cards[1].get(Field.FRONT) == "France"
    assert parsed.cards[1].get(Field.BACK) == "Paris"
    assert parsed.cards[1].get(Field.PINYIN) is None


def test_deck_offset_table_random_access():
    deck = _sample_deck()
    data = write_deck(deck)
    # Header advertises the offset table position; each entry points at a record
    # whose first four bytes are the card id.
    offset_table_pos = struct.unpack_from("<I", data, 12)[0]
    for i, card in enumerate(deck.cards):
        record_off = struct.unpack_from("<I", data, offset_table_pos + i * 4)[0]
        card_id = struct.unpack_from("<I", data, record_off)[0]
        assert card_id == card.card_id


def test_deck_no_pinyin_flag_clear():
    deck = Deck(name="Caps")
    c = Card(card_id=1)
    c.set(Field.FRONT, "Japan")
    c.set(Field.BACK, "Tokyo")
    deck.cards = [c]
    data = write_deck(deck)
    assert not (data[5] & deckformat.DECK_FLAG_HAS_PINYIN)


def test_read_deck_rejects_bad_magic():
    with pytest.raises(ValueError):
        read_deck(b"XXXX" + b"\x00" * 20)


def test_review_state_roundtrip():
    state = ReviewState(new_introduced=3, last_review_day=123, streak_days=7)
    state.records = [
        ReviewRecord(card_id=10, due_day=130, interval_days=6, easiness_milli=2400, reps=2, lapses=1, last_grade=2),
        ReviewRecord(card_id=3, due_day=125, interval_days=1, easiness_milli=2500, reps=0, lapses=0, last_grade=0),
    ]
    data = write_review_state(state)
    assert data[:4] == b"INKR"

    parsed = read_review_state(data)
    assert parsed.new_introduced == 3
    assert parsed.last_review_day == 123
    assert parsed.streak_days == 7
    # Records are written in ascending cardId order.
    assert [r.card_id for r in parsed.records] == [3, 10]
    assert parsed.records[1].easiness_milli == 2400
    assert parsed.records[1].lapses == 1
