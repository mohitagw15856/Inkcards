# Engine unit tests

Host tests for the portable InkCards engine (`../lib/inkcards_core`). They use a
tiny built-in check harness (`check.h`) so there is no external test-framework or
network dependency, just a C++17 compiler.

```sh
make            # build and run
make clean
```

`deck_builder.h` assembles a `.deck` byte buffer in memory so the `DeckReader`
can be tested against a known-good layout that mirrors `docs/FORMAT.md`. The
suite covers serialisation, the FNV deck-id hash (against canonical test
vectors), deck round-trips and random access, unknown-field tolerance, the SM-2
algorithm, review-state load/save, and the session queue and streak logic.
