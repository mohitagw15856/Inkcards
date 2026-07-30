# Contributing to InkCards

Thanks for your interest in InkCards. Contributions of all kinds are welcome:
bug reports, decks, documentation, companion features and firmware work.

## Ground rules

- Be kind and constructive.
- Documentation uses British English and avoids em dashes.
- Keep the device memory-disciplined: stream from the SD card, avoid large heap
  allocations, and pre-process heavy work in the companion tool rather than on
  the device.
- Put non-trivial logic in the portable engine (`firmware/lib/inkcards_core`) so
  it can be unit tested on the host, and keep the device layer thin.

## Project layout

See [`ARCHITECTURE.md`](ARCHITECTURE.md) for the full picture. In short:

- `firmware/lib/inkcards_core` is portable C++17 with no Arduino or SDK
  dependency. This is where the deck format, SM-2 and review state live.
- `firmware/src` is the e-ink UI on the FreeInk SDK.
- `firmware/selftest` is a self-contained firmware CI builds.
- `companion` is the Python CLI.

## Building and testing

Engine unit tests (host, needs only a C++17 compiler):

```sh
make -C firmware/test
```

Companion tests:

```sh
cd companion
pip install -e ".[dev]"
pytest
```

Firmware (PlatformIO):

```sh
cd firmware
pio run -e selftest        # self-contained, no SDK
pio run -e device          # full UI, needs the freeink-sdk submodule
```

Please make sure both test suites pass before opening a pull request. CI runs
the engine tests, the companion tests and the self-test firmware build.

## Changing the file formats

The `.deck` and `.rev` formats are a contract shared by the firmware and the
companion. If you change them:

1. Update [`docs/FORMAT.md`](docs/FORMAT.md) first; it is the source of truth.
2. Update both `firmware/lib/inkcards_core` and
   `companion/inkcards/deckformat.py` together.
3. Bump the relevant version byte and keep readers tolerant of unknown fields.
4. Add or update tests on both sides. The FNV deck-id hash and the byte layout
   are cross-checked, so they must stay in lockstep.

## Working on the device layer

Anything that cannot be verified without hardware is marked
`TODO(hardware-test)` and listed in
[`docs/HARDWARE_TESTING.md`](docs/HARDWARE_TESTING.md). If you have a device,
verifying and removing those markers is a hugely valuable contribution. If you
add device code that needs on-hardware confirmation, add a marker and a matching
checklist entry.

## Adding decks

Sample decks live in `decks/` with their CSV sources in `decks/src/`. Rebuild
them with:

```sh
python3 decks/build.py
```

Only contribute deck content you have the right to share (public-domain word
lists, your own material, or suitably licensed sources).

## Commit and pull request style

- Write clear, focused commits with descriptive messages.
- Explain the "why" in the pull request description, and note any on-hardware
  testing you did or could not do.
