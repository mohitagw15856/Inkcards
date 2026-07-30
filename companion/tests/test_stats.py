from pathlib import Path

from inkcards import stats
from inkcards.convert import write_deck_file
from inkcards.deckformat import Card, Deck, Field, ReviewRecord, ReviewState, write_review_state


def _deck(name="Test", n=5):
    deck = Deck(name=name)
    for i in range(1, n + 1):
        c = Card(card_id=i)
        c.set(Field.FRONT, f"q{i}")
        c.set(Field.BACK, f"a{i}")
        deck.cards.append(c)
    return deck


def test_today_day():
    assert stats.today_day(now=86400) == 1
    assert stats.today_day(tz_offset_seconds=8 * 3600, now=23 * 3600) == 1
    assert stats.today_day(now=0) == 0


def test_compute_stats_all_new():
    deck = _deck(n=5)
    st = stats.compute_stats(deck, None, today=100, new_per_day=3)
    assert st.total_cards == 5
    assert st.seen == 0
    assert st.new_remaining == 5
    assert st.due_today == 0
    assert st.new_available == 3  # capped by new_per_day
    assert st.streak_days == 0


def test_compute_stats_with_state():
    deck = _deck(n=5)
    state = ReviewState(new_introduced=2, last_review_day=100, streak_days=4)
    state.records = [
        ReviewRecord(1, due_day=100, interval_days=1, easiness_milli=2500, reps=1, lapses=0, last_grade=2),
        ReviewRecord(2, due_day=105, interval_days=6, easiness_milli=2500, reps=2, lapses=0, last_grade=2),
        ReviewRecord(3, due_day=99, interval_days=1, easiness_milli=2300, reps=0, lapses=1, last_grade=0),
    ]
    st = stats.compute_stats(deck, state, today=100, new_per_day=20)
    assert st.seen == 3
    assert st.new_remaining == 2
    # Cards 1 (due 100) and 3 (due 99) are due; card 2 (due 105) is not.
    assert st.due_today == 2
    assert st.streak_days == 4
    # Two new introduced today, so 18 remain in the budget but only 2 new cards left.
    assert st.new_available == 2
    # Card 2 is due in 5 days.
    assert st.upcoming.get(5) == 1


def test_gather_sd_stats(tmp_path: Path):
    sd = tmp_path
    decks_dir = sd / "inkcards" / "decks"
    state_dir = sd / "inkcards" / "state"
    decks_dir.mkdir(parents=True)
    state_dir.mkdir(parents=True)

    deck = _deck(name="Capitals", n=4)
    write_deck_file(deck, decks_dir / "capitals.deck")

    state = ReviewState(new_introduced=1, last_review_day=50, streak_days=3)
    state.records = [ReviewRecord(1, due_day=40, interval_days=1, easiness_milli=2500, reps=1, lapses=0, last_grade=2)]
    (state_dir / f"{deck.deck_id}.rev").write_bytes(write_review_state(state))

    rows = stats.gather_sd_stats(sd, today=50)
    assert len(rows) == 1
    assert rows[0].name == "Capitals"
    assert rows[0].seen == 1
    assert rows[0].due_today == 1

    table = stats.format_stats_table(rows)
    assert "Capitals" in table
    assert "Deck" in table  # header present


def test_gather_sd_stats_empty(tmp_path):
    rows = stats.gather_sd_stats(tmp_path, today=1)
    assert rows == []
    assert "No decks" in stats.format_stats_table(rows)
