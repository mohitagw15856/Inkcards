# Hardware testing checklist

The InkCards engine (`firmware/lib/inkcards_core`) is fully covered by host unit
tests, and the companion tool by pytest. The device-facing glue, however, cannot
be verified without a physical Xteink X4/X3 and the FreeInk SDK toolchain. This
file lists every item that needs on-device confirmation. Each corresponds to a
`TODO(hardware-test)` marker in the source.

When you verify an item on hardware, update the code, remove the marker, and
tick it off here.

> The raw freeink-sdk calls behind the SD-storage, `HalFile` streaming and
> HalGPIO items below now live in the shared [`inkkit`](https://github.com/mohitagw15856/inkkit)
> library, which InkCards and HabitInk both consume. Confirming an SDK call
> against real hardware there fixes it for both apps at once; the InkCards files
> named below are the thin app-specific wrappers over inkkit.

## Firmware, device build (`firmware/src`)

- [ ] **Front-button GPIO indices** (`platform/InkInput.h`). The logical
      `Front1..Front4`, `Back` and `Confirm` buttons are mapped to guessed
      `HalGPIO` indices. Confirm the physical order on both the X4 and the X3 so
      that the grade labels (Again, Hard, Good, Easy) line up left to right with
      the real buttons.
- [ ] **`HalFile` method names** (`platform/SdByteStream.h`). The reader adapter
      calls `seekSet`, `position`, `size` and `read`, matching CrossPoint's
      usage. Confirm against the installed `freeink-sdk` version.
- [ ] **Directory listing API** (`platform/InkCardsStorage.cpp`). Deck
      enumeration uses a `Storage.listDir(path, callback)` form. Confirm the
      SDK's actual directory-iteration API and adjust `listDeckFiles()`.
- [ ] **Clock accessor** (`main.cpp`, `currentDay`). Uses `HalClock::nowUnix()`.
      Confirm the SDK accessor and that the RTC is read correctly; verify the
      day rolls over at local midnight with a real timezone offset.
- [ ] **SD-card font wiring** (`main.cpp`, `setupFonts`). InkCards intends to
      load CrossPoint `.cpfont` families from the card and assign sizes to the
      four UI roles, including size-matched CJK. Wire this to the SDK's SD font
      loading (`registerSdCardFont`) and verify Latin and CJK both render.
- [ ] **Timezone and study configuration** (`main.cpp`). The timezone offset and
      new-cards-per-day are compile-time defaults; verify sensible behaviour and
      consider an on-device settings screen.
- [ ] **Partial refresh quality**. Confirm that repainting only the card band and
      issuing a fast refresh between cards looks clean, and tune
      `kFullRefreshEvery` in `InkCardsApp.h` so ghosting is cleared often enough.
- [ ] **`HalFile::isOpen()`**. `InkCardsApp::closeDeck()` calls it; confirm it
      exists on the SDK's `HalFile`.

## Firmware, self-test build (`firmware/selftest`)

- [ ] **SD chip-select pin** (`main.cpp`, `INKCARDS_SD_CS`). Set to the X4/X3 SD
      wiring so the self-test can mount the card and report decks over serial.

## End-to-end on-device checks

- [ ] Copy the sample decks to `/inkcards/decks/` and confirm they appear in the
      deck list with the correct names.
- [ ] Review a few cards, power off, power on, and confirm progress and streak
      persisted (the `.rev` file in `/inkcards/state/`).
- [ ] Install a CJK `.cpfont` family and confirm the HSK deck renders hanzi and
      pinyin correctly.
- [ ] Capture screenshots for the README's screenshots section.
