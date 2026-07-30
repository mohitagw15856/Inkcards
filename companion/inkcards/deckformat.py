"""Reading and writing of the InkCards ``.deck`` and ``.rev`` binary formats.

This mirrors ``firmware/lib/inkcards_core`` byte for byte. See ``docs/FORMAT.md``
for the authoritative specification.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass, field
from enum import IntEnum
from typing import BinaryIO

DECK_MAGIC = b"INKD"
DECK_VERSION = 1
REV_MAGIC = b"INKR"
REV_VERSION = 1

DECK_FLAG_HAS_PINYIN = 1 << 0


class Field(IntEnum):
    """Card field types, matching the on-disk identifiers."""

    FRONT = 0
    BACK = 1
    PINYIN = 2
    EXAMPLE = 3
    NOTES = 4
    HINT = 5
    TAGS = 6


class Grade(IntEnum):
    AGAIN = 0
    HARD = 1
    GOOD = 2
    EASY = 3


def fnv1a32(data: bytes) -> int:
    """32-bit FNV-1a hash. Matches ``firmware/lib/inkcards_core/Fnv.h``."""
    h = 0x811C9DC5
    for b in data:
        h ^= b
        h = (h * 0x01000193) & 0xFFFFFFFF
    return h


def deck_id(name: str) -> str:
    """Stable deck id: eight lowercase hex digits of the FNV-1a hash of the name."""
    return f"{fnv1a32(name.encode('utf-8')):08x}"


# --- varint / string helpers ------------------------------------------------


def write_varint(buf: bytearray, value: int) -> None:
    if value < 0:
        raise ValueError("varint cannot encode a negative value")
    while True:
        byte = value & 0x7F
        value >>= 7
        if value:
            buf.append(byte | 0x80)
        else:
            buf.append(byte)
            return


def write_string(buf: bytearray, text: str) -> None:
    raw = text.encode("utf-8")
    write_varint(buf, len(raw))
    buf.extend(raw)


def read_varint(data: bytes, pos: int) -> tuple[int, int]:
    result = 0
    shift = 0
    for _ in range(5):
        byte = data[pos]
        pos += 1
        result |= (byte & 0x7F) << shift
        if not (byte & 0x80):
            return result, pos
        shift += 7
    raise ValueError("overlong varint")


def read_string(data: bytes, pos: int) -> tuple[str, int]:
    length, pos = read_varint(data, pos)
    text = data[pos : pos + length].decode("utf-8")
    return text, pos + length


# --- deck model -------------------------------------------------------------


@dataclass
class Card:
    """One flashcard: a stable id and its ordered fields."""

    card_id: int
    fields: list[tuple[int, str]] = field(default_factory=list)

    def set(self, ftype: Field, value: str) -> None:
        if value is None:
            return
        self.fields.append((int(ftype), value))

    def get(self, ftype: Field) -> str | None:
        for t, v in self.fields:
            if t == int(ftype):
                return v
        return None


@dataclass
class Deck:
    name: str
    cards: list[Card] = field(default_factory=list)
    description: str = ""
    font_hint: str = ""

    @property
    def deck_id(self) -> str:
        return deck_id(self.name)

    def has_pinyin(self) -> bool:
        return any(t == int(Field.PINYIN) for c in self.cards for t, _ in c.fields)

    def field_mask(self) -> int:
        mask = 0
        for c in self.cards:
            for t, _ in c.fields:
                mask |= 1 << t
        return mask


def write_deck(deck: Deck) -> bytes:
    """Serialise a :class:`Deck` to the ``.deck`` binary format."""
    # Encode each card record first so its length (and therefore offset) is known.
    records: list[bytes] = []
    for card in deck.cards:
        rec = bytearray()
        rec += struct.pack("<I", card.card_id)
        rec.append(len(card.fields))
        for ftype, value in card.fields:
            rec.append(ftype)
            write_string(rec, value)
        records.append(bytes(rec))

    flags = DECK_FLAG_HAS_PINYIN if deck.has_pinyin() else 0

    header = bytearray()
    header += DECK_MAGIC
    header.append(DECK_VERSION)
    header.append(flags)
    header += struct.pack("<H", deck.field_mask())
    header += struct.pack("<I", len(deck.cards))
    off_table_pos_at = len(header)
    header += struct.pack("<I", 0)  # offsetTablePos, patched below
    card_data_pos_at = len(header)
    header += struct.pack("<I", 0)  # cardDataPos, patched below
    write_string(header, deck.name)
    write_string(header, deck.description)
    write_string(header, deck.font_hint)

    offset_table_pos = len(header)
    card_data_pos = offset_table_pos + 4 * len(deck.cards)
    struct.pack_into("<I", header, off_table_pos_at, offset_table_pos)
    struct.pack_into("<I", header, card_data_pos_at, card_data_pos)

    out = bytearray(header)
    cursor = card_data_pos
    offsets = []
    for rec in records:
        offsets.append(cursor)
        cursor += len(rec)
    for off in offsets:
        out += struct.pack("<I", off)
    for rec in records:
        out += rec
    return bytes(out)


def read_deck(data: bytes) -> Deck:
    """Parse a ``.deck`` buffer into a :class:`Deck` (loads every card)."""
    if data[:4] != DECK_MAGIC:
        raise ValueError("not a .deck file (bad magic)")
    version = data[4]
    if version != DECK_VERSION:
        raise ValueError(f"unsupported .deck version {version}")
    (card_count,) = struct.unpack_from("<I", data, 8)
    (offset_table_pos,) = struct.unpack_from("<I", data, 12)
    pos = 20
    name, pos = read_string(data, pos)
    description, pos = read_string(data, pos)
    font_hint, pos = read_string(data, pos)

    deck = Deck(name=name, description=description, font_hint=font_hint)
    for i in range(card_count):
        (record_off,) = struct.unpack_from("<I", data, offset_table_pos + i * 4)
        rpos = record_off
        (card_id,) = struct.unpack_from("<I", data, rpos)
        rpos += 4
        field_count = data[rpos]
        rpos += 1
        card = Card(card_id=card_id)
        for _ in range(field_count):
            ftype = data[rpos]
            rpos += 1
            value, rpos = read_string(data, rpos)
            card.fields.append((ftype, value))
        deck.cards.append(card)
    return deck


# --- review state -----------------------------------------------------------


@dataclass
class ReviewRecord:
    card_id: int
    due_day: int
    interval_days: int
    easiness_milli: int
    reps: int
    lapses: int
    last_grade: int


@dataclass
class ReviewState:
    new_introduced: int = 0
    last_review_day: int = 0
    streak_days: int = 0
    records: list[ReviewRecord] = field(default_factory=list)


def read_review_state(data: bytes) -> ReviewState:
    """Parse a ``.rev`` buffer."""
    if data[:4] != REV_MAGIC:
        raise ValueError("not a .rev file (bad magic)")
    version = data[4]
    if version != REV_VERSION:
        raise ValueError(f"unsupported .rev version {version}")
    new_introduced = struct.unpack_from("<H", data, 6)[0]
    last_review_day = struct.unpack_from("<I", data, 8)[0]
    streak_days = struct.unpack_from("<I", data, 12)[0]
    count = struct.unpack_from("<I", data, 16)[0]
    state = ReviewState(
        new_introduced=new_introduced,
        last_review_day=last_review_day,
        streak_days=streak_days,
    )
    pos = 20
    for _ in range(count):
        card_id, due_day, interval, easiness = struct.unpack_from("<IIHH", data, pos)
        reps = data[pos + 12]
        lapses = data[pos + 13]
        last_grade = data[pos + 14]
        pos += 16
        state.records.append(
            ReviewRecord(card_id, due_day, interval, easiness, reps, lapses, last_grade)
        )
    return state


def write_review_state(state: ReviewState) -> bytes:
    """Serialise a :class:`ReviewState` (records are written in cardId order)."""
    out = bytearray()
    out += REV_MAGIC
    out.append(REV_VERSION)
    out.append(0)  # reserved
    out += struct.pack("<H", state.new_introduced)
    out += struct.pack("<I", state.last_review_day)
    out += struct.pack("<I", state.streak_days)
    records = sorted(state.records, key=lambda r: r.card_id)
    out += struct.pack("<I", len(records))
    for r in records:
        out += struct.pack("<IIHH", r.card_id, r.due_day, r.interval_days, r.easiness_milli)
        out.append(r.reps & 0xFF)
        out.append(r.lapses & 0xFF)
        out.append(r.last_grade & 0xFF)
        out.append(0)  # reserved
    return bytes(out)
