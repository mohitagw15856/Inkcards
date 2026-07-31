# Architecture

InkCards is a spaced-repetition flashcard app for the Xteink X4/X3 pocket
e-reader (ESP32-C3, 4.2 inch e-ink, SD card, physical buttons). This document
records the architecture and, importantly, the decision about how InkCards
relates to the CrossPoint Reader ecosystem.

## The decision: standalone firmware built on the FreeInk SDK

Before writing any code the CrossPoint Reader source was cloned into
`/reference` and studied: its activity-driven main loop, its SD-card-first
caching under `/.crosspoint`, its `GfxRenderer` e-ink pipeline, its SD-card
`.cpfont` font system, and its MIT licence. Three options were considered.

**(a) Fork CrossPoint.** Rejected. CrossPoint is a full EPUB/TXT/XTC reader with
OPDS, a web server, WebDAV, dictionaries, KOReader sync and TLS. A flashcard
trainer needs almost none of that. Forking would mean carrying tens of
thousands of lines of unrelated code and rebasing on every upstream release,
for no benefit to a card app.

**(b) A module or patch for upstream.** Rejected as the primary shape.
CrossPoint's `SCOPE.md` and `GOVERNANCE.md` scope the project to reading, and a
full SM-2 trainer with its own persisted state is out of that scope. A large PR
adding it is unlikely to be accepted, and it would couple InkCards releases to
CrossPoint's cadence. The CrossMux fork's "Apps hub" is the natural long-term
home for a launcher-style app, so InkCards is built to be **hub-friendly** (a
self-contained application object with a simple begin/loop lifecycle) without
depending on any hub existing today.

**(c) Standalone firmware.** Chosen. InkCards ships as its own ESP32-C3
firmware, but it does **not** reinvent the hardware layer: it depends on
**[inkkit](https://github.com/mohitagw15856/inkkit)** (pinned in
`firmware/platformio.ini`), which vendors the same HAL and hardware libraries
CrossPoint uses (display, GPIO, storage, clock), and it **reuses CrossPoint's
SD-card font system unchanged**, reading
the same `.cpfont` families from `/.fonts` or `/fonts`. This satisfies the
memory-discipline and font requirements while keeping InkCards small, focused
and independently releasable.

### Licence compatibility

CrossPoint and the FreeInk SDK are MIT licensed. InkCards is MIT licensed too,
so reusing the SD-card font system and building on the SDK is fully compatible.
No CrossPoint *application* code is copied into InkCards; only the public
hardware/rendering libraries are linked, and the `.cpfont` files on the card are
consumed as data. Where the `GfxRenderer` rendering library is reused for the
device build it is vendored under its MIT licence with attribution.

## Layers

InkCards is split so that the hard logic is portable and testable, and only a
thin layer touches hardware.

```
+-----------------------------------------------------------+
|  companion/ (Python CLI)                                  |
|  convert Anki/CSV -> .deck   |   stats from .rev on SD    |
+-----------------------------------------------------------+
                     |  .deck / .rev files on SD card
                     v
+-----------------------------------------------------------+
|  firmware/lib/inkcards_core  (portable C++17, no Arduino) |
|  DeckReader | Sm2 | ReviewState | Session | serialisation |
+-----------------------------------------------------------+
        ^                                   ^
        | host unit tests (CI)              | device glue
        |                                   v
+------------------+   +------------------------------------+
| firmware/test    |   | firmware/src  (device app)         |
| dependency-free  |   | InkCardsApp | ui/ | platform/      |
| g++ test runner  |   | GfxRenderer + HalGPIO + HalStorage |
+------------------+   +------------------------------------+
```

### `firmware/lib/inkcards_core` (the engine)

Pure C++17 with no Arduino or SDK dependency, so it compiles unchanged on the
device and on a desktop for unit testing. It contains:

- **`DeckReader`**: streams a `.deck` file, seeking to any card by index via the
  on-disk offset table. Peak card-content RAM is a single card.
- **`Sm2`**: the SM-2 scheduling algorithm as a pure function (see
  `docs/SPACED_REPETITION.md`).
- **`ReviewState`**: the per-deck `.rev` model, loaded and saved through
  abstract byte streams.
- **`Session`**: builds today's review queue (due cards then new cards, capped
  by a daily new-card budget) and applies grades, updating the streak and
  per-day counters.
- **`ByteStream` / `Serialization`**: the I/O abstraction and little-endian,
  varint and length-prefixed-string helpers that both the device and the
  companion agree on.

Every non-trivial rule lives here, which is why the engine is covered by host
unit tests that run on every push (see `firmware/test`).

### `firmware/src` (the device application)

The e-ink UI, built against the FreeInk SDK and `GfxRenderer`:

- **`InkCardsApp`**: the state machine (deck list, session summary, review,
  done). One deck is open at a time; card content is streamed from SD on demand.
- **`ui/`**: stateless draw routines. The card sits in a fixed band so it can be
  repainted and fast-refreshed on its own between cards, which is the
  partial-refresh-friendly path on this e-ink SDK. A full refresh is issued
  periodically to clear ghosting.
- **`platform/`**: adapters that expose the SDK's `HalFile` as the engine's
  byte streams, a logical button layer over `HalGPIO`, and SD path helpers.

Anything that genuinely needs a device to verify (exact GPIO indices, the
`.cpfont` loading calls, the clock accessor) is marked `TODO(hardware-test)` and
listed in `docs/HARDWARE_TESTING.md`.

### `firmware/selftest` (the CI-built firmware)

A self-contained ESP32-C3 image that links the engine and, using only the
Arduino core plus the SD library, reports each deck's card count and today's
due/new figures over serial. It needs no proprietary SDK, so CI builds it on
every push as a real firmware artefact, proving the engine cross-compiles and
fits on the target. See `firmware/platformio.ini` for the environment split
(`selftest` vs `device`).

### `companion/` (the desktop tool)

A dependency-free Python CLI (`inkcards`) that converts Anki `.apkg` exports and
CSV files into `.deck` files (with optional pinyin handling for Chinese decks),
and reads `.rev` files off an SD card to print progress. Heavy work (parsing
Anki, normalising pinyin, laying out fields) happens here on the desktop, never
on the device. The format writer mirrors the C++ reader byte for byte; the FNV
deck-id hash and the binary layout are shared and cross-checked.

## Memory discipline

The device is the constrained party, so:

- **Deck content is streamed.** `DeckReader` seeks to one card at a time using a
  four-byte offset read; the offset table is never loaded whole.
- **Only small state is held in RAM.** The review schedule is fixed 16-byte
  records; a 1000-card deck is under 16 KB, and the format is sorted so a future
  streaming variant could avoid even that.
- **Heavy work is pre-computed on the desktop.** Anki parsing, HTML stripping
  and pinyin conversion happen in the companion, so the device only ever reads
  clean, compact `.deck` records.
- **Fonts are not embedded.** InkCards loads CrossPoint's SD-card `.cpfont`
  families instead of shipping glyphs in flash, including large CJK sets.

## Data on the SD card

```
/inkcards/decks/*.deck      immutable card content (from the companion)
/inkcards/state/<id>.rev     mutable review state (written on the device)
/.fonts or /fonts            CrossPoint-compatible .cpfont families (shared)
```

Formats are specified in `docs/FORMAT.md`.

## Compatibility

InkCards is designed to sit alongside CrossPoint on the same SD card and to use
the same fonts. It targets the CrossPoint **v1.5.x** era conventions (SD-card
font layout and `.cpfont` format). See the README's compatibility note.
