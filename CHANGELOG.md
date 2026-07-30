# Changelog

All notable changes to InkCards are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project aims to follow [Semantic Versioning](https://semver.org/).

## [Unreleased]

Nothing yet.

## [0.1.0] - 2026-07-30

The first InkCards release: a complete flashcard app for the Xteink X4/X3, a
companion tool, and sample decks.

### Added

- **Portable engine** (`firmware/lib/inkcards_core`): a streaming `.deck`
  reader that seeks to any card by index while holding only one card in RAM, the
  SM-2 spaced-repetition scheduler, per-deck review state, and the study-session
  queue with a daily new-card budget and study streak. Pure C++17, covered by a
  dependency-free host test suite.
- **Device application** (`firmware/src`): the e-ink UI on the FreeInk SDK, with
  a deck list, a session summary (cards due, new cards, reviewed, streak), and a
  review flow. The four front buttons map to Again, Hard, Good and Easy, and the
  card area is repainted on its own between cards for a partial-refresh-friendly
  update.
- **Self-test firmware** (`firmware/selftest`): a self-contained ESP32-C3 image
  that links the engine and reports decks over serial, built by CI.
- **Companion CLI** (`companion`): `inkcards convert` for Anki `.apkg` and CSV
  input (with pinyin tone-mark conversion for Chinese decks) and `inkcards
  stats` for reading progress off an SD card. Covered by pytest.
- **Binary formats**: the `.deck` and `.rev` formats, specified in
  `docs/FORMAT.md` and shared byte for byte by the firmware and the companion.
- **Sample decks**: HSK 1 vocabulary (150 cards, with pinyin), capitals of the
  world (195 cards), and a README demo deck, built reproducibly from CSV.
- **SD-card font support**: CJK and other scripts render from CrossPoint
  compatible `.cpfont` families rather than embedded fonts.
- **Documentation**: architecture and the standalone-firmware decision,
  spaced-repetition notes, the button mapping, a hardware-testing checklist, and
  contributor guides.
- **Project health**: MIT licence, contributing guide, security policy, issue
  and pull-request templates, a logo, a README banner, UI mockups and a social
  preview image.
- **Continuous integration**: engine unit tests, companion tests, and the
  self-test firmware build.

### Notes

- The device UI is written against the FreeInk SDK and is pending on-hardware
  verification; see `docs/HARDWARE_TESTING.md` for the checklist.

[Unreleased]: https://github.com/mohitagw15856/inkcards/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/mohitagw15856/inkcards/releases/tag/v0.1.0
