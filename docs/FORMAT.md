# InkCards file formats

This document specifies the on-SD-card file formats used by InkCards. There are
two families of file:

- **`.deck`** files: the immutable card content, produced on a desktop by the
  companion tool and copied to the device. Optimised for streaming and random
  access by card index without loading the whole deck into RAM.
- **`.rev`** files: the mutable per-deck review state, written on the device as
  you study. Small (one record per card that has been seen at least once).

Both formats are little-endian, which matches the ESP32-C3 and every desktop
platform the companion runs on. All multi-byte integers are stored
little-endian. Strings are UTF-8 and are length-prefixed (never NUL
terminated), so any byte value is allowed inside a field, including embedded
newlines.

The design goal throughout is **memory discipline**: the device never needs to
hold more than one card in RAM at a time, and it can seek directly to card *N*
without scanning the cards before it.

## Conventions

`varint`
: An unsigned LEB128 variable-length integer. Seven bits of payload per byte,
  little-endian, the high bit set on every byte except the last. Values 0 to
  127 take one byte. Used for field lengths, which are almost always small.

`u8`, `u16`, `u32`
: Unsigned little-endian integers of 1, 2 and 4 bytes.

`string`
: A `varint` byte length followed by that many bytes of UTF-8 text.

## `.deck` format, version 1

A `.deck` file has three regions laid out in this order:

```
+------------------+  offset 0
| Header           |
+------------------+
| Offset table     |   cardCount x u32
+------------------+
| Card records     |   variable length, back to back
+------------------+
```

### Header

| Field            | Type     | Notes                                            |
| ---------------- | -------- | ------------------------------------------------ |
| magic            | 4 bytes  | ASCII `I N K D` (`0x49 0x4E 0x4B 0x44`)          |
| version          | u8       | `1`                                              |
| flags            | u8       | bit 0: deck contains a pinyin field. Others 0.   |
| fieldMask        | u16      | bitmask of field types present anywhere in deck. |
| cardCount        | u32      | number of cards, also number of offset entries.  |
| offsetTablePos   | u32      | absolute file offset of the offset table.        |
| cardDataPos      | u32      | absolute file offset of the first card record.   |
| name             | string   | human-readable deck name.                        |
| description      | string   | free text, may be empty.                         |
| fontHint         | string   | suggested SD-card font family, may be empty.     |

`fontHint` is advisory. It names a CrossPoint SD-card font family (for example
`NotoSansSC`) that renders this deck's script well. The device uses it to pick a
sensible default when the deck contains CJK, but the reader can always override
it. See [SD card fonts](#sd-card-fonts-and-cjk) below.

`offsetTablePos` and `cardDataPos` are stored explicitly so a reader can jump
straight to the offset table without re-parsing the variable-length strings in
the header on every open.

### Offset table

Immediately at `offsetTablePos`: `cardCount` entries of `u32`, each the
absolute file offset of the corresponding card record. To read card *N*, seek
to `offsetTablePos + N * 4`, read one `u32`, then seek to that offset. The
device never loads the whole table; it reads four bytes on demand. A desktop
tool may of course load it whole.

### Card record

Each record is self-describing:

| Field       | Type    | Notes                                             |
| ----------- | ------- | ------------------------------------------------- |
| cardId      | u32     | stable identifier, matched against `.rev` records.|
| fieldCount  | u8      | number of fields that follow.                     |
| fields      | ...     | `fieldCount` field entries.                       |

Each field entry:

| Field   | Type    | Notes                                    |
| ------- | ------- | ---------------------------------------- |
| type    | u8      | field type, see table below.             |
| value   | string  | varint length prefixed UTF-8.            |

Field types:

| Value | Name    | Meaning                                             |
| ----- | ------- | --------------------------------------------------- |
| 0     | FRONT   | the prompt (e.g. the hanzi, or the country).        |
| 1     | BACK    | the answer (e.g. the English gloss, or the capital).|
| 2     | PINYIN  | romanised reading, shown under the front for CJK.   |
| 3     | EXAMPLE | example sentence or usage.                           |
| 4     | NOTES   | extra note revealed with the answer.                |
| 5     | HINT    | short hint shown before the answer.                 |
| 6     | TAGS    | comma-separated tags.                               |

A well-formed card has at least a FRONT and a BACK. Unknown field types must be
skipped gracefully by readers (read the length, skip the bytes) so the format
can grow without breaking old firmware.

`cardId` is assigned by the companion tool. By convention it is the 1-based
position of the card in the source, but any deck-unique 32-bit value is legal.
Keeping it stable across re-exports lets a reader preserve review history when a
deck is edited and reconverted.

### Why this shape

- **Streaming**: cards are read one at a time straight off the SD card. Peak
  card-content RAM is one card.
- **Random access**: the offset table gives O(1) seek to any card, which the
  scheduler needs because due cards are visited in review order, not file order.
- **Forward compatible**: self-describing fields and an explicit version byte.
- **Compact**: varint lengths keep the per-field overhead to a single byte for
  the common short-string case.

## `.rev` format, version 1

Review state lives next to a small header and then one fixed-size record per
card that has been graded at least once. Cards never seen are simply absent;
they are "new" and introduced up to the daily new-card limit.

Stored at `/inkcards/state/<deckId>.rev` where `<deckId>` is the deck's stable
id (see [Deck identity](#deck-identity)).

### Header

| Field         | Type    | Notes                                             |
| ------------- | ------- | ------------------------------------------------- |
| magic         | 4 bytes | ASCII `I N K R` (`0x49 0x4E 0x4B 0x52`)           |
| version       | u8      | `1`                                               |
| reserved      | u8      | `0`                                               |
| newIntroduced | u16     | new cards introduced during `lastReviewDay`.      |
| lastReviewDay | u32     | day number (see below) of the most recent review.|
| streakDays    | u32     | current consecutive-day study streak.             |
| recordCount   | u32     | number of card records that follow.               |

"Day number" is the number of whole days since the Unix epoch in the device's
local time zone: `floor((unixTime + tzOffsetSeconds) / 86400)`. Using a day
number rather than a timestamp makes "due today" a simple integer comparison
and avoids intraday scheduling churn on a device whose clock may only be
roughly set.

### Card record (fixed 16 bytes)

| Field      | Type    | Notes                                                     |
| ---------- | ------- | --------------------------------------------------------- |
| cardId     | u32     | matches `cardId` in the `.deck`.                          |
| dueDay     | u32     | day number on which this card next becomes due.           |
| intervalD  | u16     | current inter-repetition interval in days.                |
| easiness   | u16     | SM-2 easiness factor times 1000 (e.g. 2500 = 2.5).        |
| reps       | u8      | number of consecutive correct repetitions.               |
| lapses     | u8      | number of times the card has been forgotten.             |
| lastGrade  | u8      | last grade applied (0 Again, 1 Hard, 2 Good, 3 Easy).    |
| reserved   | u8      | `0`.                                                      |

Records are written in ascending `cardId` order so a reader can binary-search
the file for a card without loading it all. The whole file for a 1000-card deck
is under 16 KB, but the format never requires holding more than the header plus
one record in RAM.

## Deck identity

A deck's stable id is the lowercase hexadecimal FNV-1a 32-bit hash of the
deck's `name` string as stored in the header. The companion prints it
(`inkcards stats` uses it to pair a `.deck` with its `.rev`), and the firmware
computes it the same way. The 32-bit value is rendered as eight hex digits,
for example `1a2b3c4d`.

FNV-1a 32-bit, for reference:

```
hash = 0x811c9dc5
for each byte b of the UTF-8 name:
    hash = hash XOR b
    hash = (hash * 0x01000193) mod 2^32
```

## SD card layout

InkCards uses a single visible directory so the files are easy to manage from a
desktop, mirroring CrossPoint's convention of a predictable on-card layout:

```
SD Card Root/
  inkcards/
    decks/
      hsk1.deck
      world-capitals.deck
      demo.deck
    state/
      1a2b3c4d.rev        <- review state, one per deck, keyed by deck id
```

Fonts are **not** stored here. InkCards reuses CrossPoint's SD-card font
system unchanged (see below).

## SD card fonts and CJK

InkCards does not embed CJK fonts. It reuses the exact `.cpfont` families that
CrossPoint loads from `/.fonts/` (or `/fonts/`) on the SD card, so a single
CJK-capable family installed for CrossPoint also serves InkCards. The `fontHint`
field in a `.deck` names the family a deck prefers. See
[docs/sd-card-fonts in CrossPoint](https://github.com/crosspoint-reader/crosspoint-reader/blob/master/docs/sd-card-fonts.md)
for how to install and convert fonts, and `ARCHITECTURE.md` for how InkCards
consumes them.
