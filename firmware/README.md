# InkCards firmware

ESP32-C3 firmware for the Xteink X4/X3, built with PlatformIO.

## Structure

- `lib/inkcards_core/` The portable engine: `.deck` streaming reader, SM-2,
  review state, session queue and serialisation. Pure C++17, no Arduino or SDK
  dependency, so it compiles on the device and on a host for testing.
- `src/` The e-ink application (the `device` environment): the app state
  machine, UI draw routines and platform adapters, built on the FreeInk SDK and
  `GfxRenderer`.
- `selftest/` A self-contained engine self-test firmware (the `selftest`
  environment), using only the Arduino core and SD library. CI builds this.
- `test/` Host unit tests for the engine.

## Environments

See `platformio.ini`.

```sh
# Self-contained engine self-test firmware (default, no external SDK).
pio run -e selftest
pio run -e selftest -t upload
pio device monitor

# Full e-ink UI (needs the freeink-sdk submodule and the GfxRenderer library).
pio run -e device
pio run -e device -t upload
```

## Host tests

No board or toolchain required, just a C++17 compiler:

```sh
make -C test
```

The same tests are compiled and run in CI.

## On-device status

The engine is fully unit tested. The device layer is written against the
FreeInk SDK APIs studied from CrossPoint and is pending on-hardware
verification; see [`../docs/HARDWARE_TESTING.md`](../docs/HARDWARE_TESTING.md)
for the checklist and the `TODO(hardware-test)` markers in the source.
