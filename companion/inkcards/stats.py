"""Read InkCards review state off an SD card and summarise progress."""

from __future__ import annotations

import time
from dataclasses import dataclass, field
from pathlib import Path

from .deckformat import Deck, ReviewState, read_deck, read_review_state


def today_day(tz_offset_seconds: int = 0, now: float | None = None) -> int:
    """Current local day number (see docs/FORMAT.md)."""
    if now is None:
        now = time.time()
    local = int(now) + tz_offset_seconds
    if local < 0:
        local = 0
    return local // 86400


@dataclass
class DeckStats:
    name: str
    deck_id: str
    total_cards: int
    seen: int
    new_remaining: int
    due_today: int
    new_available: int
    streak_days: int
    last_review_day: int
    upcoming: dict[int, int] = field(default_factory=dict)  # day-offset -> count


def compute_stats(
    deck: Deck,
    state: ReviewState | None,
    today: int,
    *,
    new_per_day: int = 20,
    horizon: int = 7,
) -> DeckStats:
    """Compute progress figures for one deck given its review state."""
    card_ids = {c.card_id for c in deck.cards}
    records = [r for r in (state.records if state else []) if r.card_id in card_ids]
    seen_ids = {r.card_id for r in records}

    seen = len(seen_ids)
    total = len(deck.cards)
    new_remaining = total - seen

    due_today = sum(1 for r in records if r.due_day <= today)

    introduced_today = state.new_introduced if state and state.last_review_day == today else 0
    new_available = max(0, min(new_remaining, new_per_day - introduced_today))

    upcoming: dict[int, int] = {}
    for r in records:
        offset = r.due_day - today
        if 0 < offset <= horizon:
            upcoming[offset] = upcoming.get(offset, 0) + 1

    return DeckStats(
        name=deck.name,
        deck_id=deck.deck_id,
        total_cards=total,
        seen=seen,
        new_remaining=new_remaining,
        due_today=due_today,
        new_available=new_available,
        streak_days=state.streak_days if state else 0,
        last_review_day=state.last_review_day if state else 0,
        upcoming=upcoming,
    )


def _load_state_for(sd_root: Path, deck: Deck) -> ReviewState | None:
    state_path = sd_root / "inkcards" / "state" / f"{deck.deck_id}.rev"
    if not state_path.exists():
        return None
    try:
        return read_review_state(state_path.read_bytes())
    except (ValueError, IndexError):
        return None


def gather_sd_stats(sd_root: str | Path, today: int, *, new_per_day: int = 20) -> list[DeckStats]:
    """Load every deck under ``<sd_root>/inkcards/decks`` and pair it with state."""
    sd_root = Path(sd_root)
    decks_dir = sd_root / "inkcards" / "decks"
    results: list[DeckStats] = []
    if not decks_dir.is_dir():
        return results
    for deck_path in sorted(decks_dir.glob("*.deck")):
        try:
            deck = read_deck(deck_path.read_bytes())
        except (ValueError, IndexError):
            continue
        state = _load_state_for(sd_root, deck)
        results.append(compute_stats(deck, state, today, new_per_day=new_per_day))
    return results


def format_stats_table(rows: list[DeckStats]) -> str:
    """Render deck stats as a plain-text table."""
    if not rows:
        return "No decks found under inkcards/decks."

    headers = ["Deck", "Cards", "Seen", "New", "Due", "Streak"]
    table = [headers]
    for r in rows:
        table.append(
            [
                r.name,
                str(r.total_cards),
                str(r.seen),
                str(r.new_remaining),
                str(r.due_today),
                f"{r.streak_days}d",
            ]
        )

    widths = [max(len(row[i]) for row in table) for i in range(len(headers))]
    lines = []
    for idx, row in enumerate(table):
        line = "  ".join(cell.ljust(widths[i]) for i, cell in enumerate(row))
        lines.append(line.rstrip())
        if idx == 0:
            lines.append("  ".join("-" * widths[i] for i in range(len(headers))))
    return "\n".join(lines)
