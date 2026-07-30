# InkCards companion

A dependency-free Python CLI for InkCards: convert decks and inspect review
progress. Requires Python 3.9 or newer.

## Install

```sh
cd companion
pip install -e .           # or: pip install -e ".[dev]" for the test tools
```

This installs the `inkcards` command.

## Convert

Turn a CSV or an Anki `.apkg` export into a `.deck` file.

```sh
# CSV: columns are auto-detected from common header names.
inkcards convert capitals.csv -o world-capitals.deck --name "Capitals"

# A Chinese CSV: hanzi,pinyin,english is recognised automatically; numbered
# pinyin (ni3 hao3) is converted to tone marks (nǐ hǎo).
inkcards convert hsk1.csv -o hsk1.deck --name "HSK 1" --font-hint NotoSansSC

# Anki export: fields are mapped by note-type field name.
inkcards convert MyDeck.apkg -o mydeck.deck
```

Useful options:

- `--front-col`, `--back-col`, `--pinyin-col`, `--example-col`, `--notes-col`,
  `--hint-col`, `--tags-col`: map CSV columns explicitly.
- `--delimiter`: CSV delimiter (a `.tsv` input uses tab automatically).
- `--front-field`, `--back-field`, `--pinyin-field`: map Anki fields explicitly.
- `--name`, `--description`, `--font-hint`: deck metadata.

### Anki notes

`.apkg` files exported with "Support older Anki versions" enabled contain a
readable collection and convert directly. The newest zstd-compressed export
(`collection.anki21b` only) is not supported; re-export with the compatibility
option enabled.

## Stats

Read review state off an SD card and print progress.

```sh
inkcards stats --sd /Volumes/SDCARD
inkcards stats --sd /Volumes/SDCARD --tz-offset 28800   # local time, UTC+8

# Or inspect a single deck plus its state file.
inkcards stats --deck hsk1.deck --state 1a2b3c4d.rev
```

## Formats

The `.deck` and `.rev` binary formats are specified in
[`../docs/FORMAT.md`](../docs/FORMAT.md) and implemented in
`inkcards/deckformat.py`, matching the firmware engine byte for byte.

## Tests

```sh
pip install -e ".[dev]"
pytest
```
