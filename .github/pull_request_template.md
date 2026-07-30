<!--
Thanks for contributing to InkCards! Please fill in the sections below.
Keep documentation in British English with no em dashes.
-->

## Summary

<!-- What does this change do, and why? -->

## Type of change

- [ ] Bug fix
- [ ] New feature
- [ ] Documentation
- [ ] Refactor / cleanup
- [ ] Deck or content

## Component

- [ ] Firmware engine (`firmware/lib/inkcards_core`)
- [ ] Firmware device app (`firmware/src`)
- [ ] Self-test firmware (`firmware/selftest`)
- [ ] Companion CLI (`companion`)
- [ ] Docs / decks / CI

## Checklist

- [ ] Engine tests pass: `make -C firmware/test`
- [ ] Companion tests pass: `cd companion && pytest`
- [ ] If I changed a file format, I updated `docs/FORMAT.md` and both the C++
      and Python sides together.
- [ ] New device code that needs a real device is marked `TODO(hardware-test)`
      and listed in `docs/HARDWARE_TESTING.md`.
- [ ] I kept the device memory-disciplined (stream from SD, no large heap
      allocations, heavy work done in the companion).

## Testing notes

<!-- What did you test, and on what (host, self-test firmware, real device)?
     Note anything you could not verify without hardware. -->
