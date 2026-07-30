from pathlib import Path

from inkcards.cli import main
from inkcards.deckformat import Field, read_deck


def test_cli_convert_csv(tmp_path, capsys):
    src = tmp_path / "caps.csv"
    src.write_text("country,capital\nFrance,Paris\nJapan,Tokyo\n", encoding="utf-8")
    out = tmp_path / "caps.deck"

    rc = main(["convert", str(src), "-o", str(out), "--name", "Capitals"])
    assert rc == 0
    assert out.exists()
    deck = read_deck(out.read_bytes())
    assert deck.name == "Capitals"
    assert deck.cards[0].get(Field.BACK) == "Paris"

    captured = capsys.readouterr()
    assert "Wrote" in captured.out


def test_cli_convert_default_output(tmp_path):
    src = tmp_path / "d.csv"
    src.write_text("front,back\na,1\n", encoding="utf-8")
    rc = main(["convert", str(src)])
    assert rc == 0
    assert (tmp_path / "d.deck").exists()


def test_cli_convert_missing_input(tmp_path, capsys):
    rc = main(["convert", str(tmp_path / "nope.csv")])
    assert rc == 2
    assert "not found" in capsys.readouterr().err


def test_cli_stats_sd(tmp_path, capsys):
    # Build a deck on a fake SD card and run stats over it.
    from inkcards.convert import write_deck_file
    from inkcards.deckformat import Card, Deck

    decks = tmp_path / "inkcards" / "decks"
    decks.mkdir(parents=True)
    deck = Deck(name="Demo")
    c = Card(card_id=1)
    c.set(Field.FRONT, "q")
    c.set(Field.BACK, "a")
    deck.cards.append(c)
    write_deck_file(deck, decks / "demo.deck")

    rc = main(["stats", "--sd", str(tmp_path)])
    assert rc == 0
    assert "Demo" in capsys.readouterr().out


def test_cli_stats_requires_target(capsys):
    rc = main(["stats"])
    assert rc == 2
    assert "pass --sd" in capsys.readouterr().err
