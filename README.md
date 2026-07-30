# InkCards

InkCards is an open-source spaced-repetition flashcard app for the
[Xteink X4/X3](https://github.com/crosspoint-reader/crosspoint-reader) pocket
e-reader. Study vocabulary, capitals, or any deck you like on a distraction-free
e-ink screen with physical buttons, using the proven SM-2 algorithm.

It is built for the same hardware as, and lives happily alongside,
[CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader): it
runs on the FreeInk SDK and reuses CrossPoint's SD-card font system, so a single
CJK font installed for CrossPoint also renders your Mandarin decks in InkCards.

## Features

On the device:

- Loads decks from the SD card at `/inkcards/decks/*.deck`, a compact binary
  format optimised for streaming (documented in
  [`docs/FORMAT.md`](docs/FORMAT.md)).
- SM-2 spaced repetition with per-deck review state saved on the SD card.
- Four-button grading: **Again / Hard / Good / Easy**, mapped left to right to
  the four front buttons (see [`docs/BUTTON_MAPPING.md`](docs/BUTTON_MAPPING.md)).
- A session screen showing cards due today, new cards, cards reviewed and your
  study streak.
- Full Unicode including CJK, using CrossPoint's SD-card `.cpfont` fonts rather
  than embedding fonts in flash.
- A partial-refresh-friendly UI that repaints only the card area between cards.

On the desktop (the `inkcards` companion CLI):

- `inkcards convert` turns Anki `.apkg` exports and plain CSV into `.deck`
  files, including optional pinyin handling for Chinese decks.
- `inkcards stats` reads review-state files off the SD card and prints your
  progress.

## Screenshots

<!-- Screenshots placeholder. Add device photos and companion output here once
     captured on hardware (see docs/HARDWARE_TESTING.md). -->

| Deck list | Reviewing a card | Session summary |
| --------- | ---------------- | --------------- |
| _todo_    | _todo_           | _todo_          |

## Compatibility

Works with **CrossPoint v1.5.x** conventions: the SD-card font layout
(`/.fonts` or `/fonts`) and the `.cpfont` font format. InkCards reads those
fonts directly; it does not modify CrossPoint's files and can share the same SD
card. See [`ARCHITECTURE.md`](ARCHITECTURE.md) for how the two relate.

## Repository layout

```
firmware/     ESP32-C3 firmware (PlatformIO)
  lib/inkcards_core/   portable engine: deck format, SM-2, review state
  src/                 e-ink UI (FreeInk SDK)
  selftest/            self-contained engine self-test firmware (CI-built)
  test/                host unit tests for the engine
companion/    Python CLI: convert decks, read stats
decks/        sample decks and their CSV sources
docs/         format, button mapping, spaced repetition, hardware testing
```

## Quick start: the companion tool

Requires Python 3.9 or newer. No third-party dependencies.

```sh
cd companion
pip install -e .

# Convert a CSV of country,capital pairs into a deck.
inkcards convert my-capitals.csv -o world-capitals.deck --name "Capitals"

# Convert an Anki export, treating a Pinyin field specially.
inkcards convert HSK1.apkg -o hsk1.deck --name "HSK 1" --font-hint NotoSansSC

# See your progress from an inserted SD card.
inkcards stats --sd /Volumes/SDCARD
```

The three sample decks in `decks/` are built from the CSVs in `decks/src/` with:

```sh
python3 decks/build.py
```

## Installing decks and fonts on the SD card

```
SD Card Root/
  inkcards/
    decks/
      hsk1.deck
      world-capitals.deck
      demo.deck
  .fonts/               (optional) CrossPoint-compatible CJK font for CJK decks
    NotoSansSC/
      NotoSansSC_16.cpfont
      ...
```

Copy `.deck` files into `/inkcards/decks/`. For CJK decks, install a
CJK-capable `.cpfont` family exactly as you would for CrossPoint (see
CrossPoint's
[SD card fonts guide](https://github.com/crosspoint-reader/crosspoint-reader/blob/master/docs/sd-card-fonts.md)).
The device creates `/inkcards/state/` for review progress on first use.

## Building and flashing the firmware

InkCards uses [PlatformIO](https://platformio.org/).

Two build environments are provided (see `firmware/platformio.ini`):

- **`selftest`** (default): a self-contained ESP32-C3 image that validates the
  deck engine over serial, with no external SDK. This is what CI builds.

  ```sh
  cd firmware
  pio run -e selftest              # build
  pio run -e selftest -t upload    # flash over USB
  pio device monitor               # watch the self-test report decks on your card
  ```

- **`device`**: the full e-ink UI, built against the FreeInk SDK. Check out the
  `freeink-sdk` submodule and provide the `GfxRenderer` library (both MIT), then:

  ```sh
  cd firmware
  pio run -e device
  pio run -e device -t upload
  ```

  The device UI has been written against the FreeInk SDK APIs and is pending
  on-hardware verification; see [`docs/HARDWARE_TESTING.md`](docs/HARDWARE_TESTING.md).

## Running the tests

Engine unit tests (host, no board or toolchain needed):

```sh
make -C firmware/test
```

Companion tests:

```sh
cd companion
pip install -e ".[dev]"
pytest
```

## Documentation

- [`ARCHITECTURE.md`](ARCHITECTURE.md): the fork-versus-standalone decision and
  the layered design.
- [`docs/FORMAT.md`](docs/FORMAT.md): the `.deck` and `.rev` binary formats.
- [`docs/BUTTON_MAPPING.md`](docs/BUTTON_MAPPING.md): the physical button layout.
- [`docs/SPACED_REPETITION.md`](docs/SPACED_REPETITION.md): the SM-2 algorithm
  and grade mapping.
- [`docs/HARDWARE_TESTING.md`](docs/HARDWARE_TESTING.md): items to verify on a
  real device.
- [`CONTRIBUTING.md`](CONTRIBUTING.md): how to build, test and contribute.

## Licence

MIT. See [`LICENSE`](LICENSE).
