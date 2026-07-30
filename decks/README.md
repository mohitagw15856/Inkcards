# Sample decks

Three ready-to-use decks, built from the CSV sources in `src/`.

| Deck                    | Cards | Notes                                             |
| ----------------------- | ----- | ------------------------------------------------- |
| `hsk1.deck`             | 150   | HSK 1 Mandarin vocabulary, with pinyin tone marks. Suggests the `NotoSansSC` SD font. |
| `world-capitals.deck`   | 195   | Sovereign countries and their capital cities.     |
| `demo.deck`             | 5     | The small demo deck shown in the README.          |

## Rebuilding

The `.deck` files are generated from `src/*.csv` by the companion tool:

```sh
python3 decks/build.py
```

This imports the `inkcards` companion package, so either install it
(`pip install -e companion`) or rely on the script adding `companion/` to the
path, which it does automatically.

## Installing on a device

Copy the `.deck` files into `/inkcards/decks/` on the SD card. For the HSK deck,
install a CJK-capable `.cpfont` font family as described in the top-level README.

## Sources and licensing

- The HSK 1 word list is a published list of vocabulary items (factual data).
- The capitals list is factual geographic data.
- The demo deck is original content for this project.

All are freely redistributable under this repository's MIT licence.
