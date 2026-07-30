"""Pinyin helpers: convert numbered pinyin (``ni3 hao3``) to tone marks
(``nǐ hǎo``).

Decks may arrive with pinyin in either style. The converter normalises numbered
pinyin to diacritics so the device shows proper tone marks; text that already
uses diacritics (or has no tone numbers) is passed through untouched.
"""

from __future__ import annotations

import re

# base vowel -> string indexed by tone (0/neutral, 1, 2, 3, 4).
_TONE_MARKS = {
    "a": "aāáǎà",
    "e": "eēéěè",
    "i": "iīíǐì",
    "o": "oōóǒò",
    "u": "uūúǔù",
    "ü": "üǖǘǚǜ",
}

# A run of pinyin letters (v and : stand in for ü) followed by a tone digit.
_SYLLABLE = re.compile(r"([a-zA-Zü]+)([0-5])")


def _accent_syllable(letters: str, tone: int) -> str:
    text = letters.replace("v", "ü").replace("V", "Ü")
    if tone in (0, 5):
        return text

    lower = text.lower()
    idx = -1
    if "a" in lower:
        idx = lower.index("a")
    elif "e" in lower:
        idx = lower.index("e")
    elif "ou" in lower:
        idx = lower.index("o")
    else:
        for i, ch in enumerate(lower):
            if ch in "aeiouü":
                idx = i
    if idx < 0:
        return text

    ch = text[idx]
    marks = _TONE_MARKS.get(ch.lower())
    if not marks:
        return text
    accented = marks[tone]
    if ch.isupper():
        accented = accented.upper()
    return text[:idx] + accented + text[idx + 1 :]


def is_numbered(text: str) -> bool:
    """True if the text contains at least one numbered-pinyin syllable."""
    return _SYLLABLE.search(text) is not None


def numbered_to_diacritics(text: str) -> str:
    """Convert every numbered-pinyin syllable in ``text`` to tone marks.

    Syllables without a trailing tone digit (already diacritic, or neutral tone)
    are left as they are, so this is safe to call on any pinyin string.
    """

    def repl(match: re.Match[str]) -> str:
        return _accent_syllable(match.group(1), int(match.group(2)))

    return _SYLLABLE.sub(repl, text)


def normalise(text: str) -> str:
    """Normalise a pinyin string to tone marks, trimming surrounding whitespace."""
    return numbered_to_diacritics(text.strip())
